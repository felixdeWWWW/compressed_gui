#include "stb_image.h"
#include <turbojpeg.h>

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include <cmath>
#include <map>
#include "helpers.h"

#include <filesystem>
#include <iomanip>
#include <tuple>




class JpgEncoder
{
private:
    const char* path_in;
    const char* path_out;
    int width, height, channels;
    unsigned char* data;
    unsigned char* data_with_alpha;
public:

    JpgEncoder(const char* path_in, const char* path_out)
        : path_in(path_in), path_out(path_out), data(nullptr), data_with_alpha(nullptr)
    {
        // Load with 4 channels (RGBA)
        data_with_alpha = stbi_load(path_in, &width, &height, &channels, 4);
        if (!data_with_alpha) {
            std::cerr << "Failed to load image\n";
            throw - 1;
        }

        // Allocate RGB buffer
        data = new unsigned char[width * height * 3];
    }

    ~JpgEncoder()
    {
        if (data) delete[] data;
        if (data_with_alpha) stbi_image_free(data_with_alpha);
    }

    void applyAlphaMask()
    {
        for (int i = 0; i < width * height; ++i) {
            unsigned char r = data_with_alpha[i * 4 + 0];
            unsigned char g = data_with_alpha[i * 4 + 1];
            unsigned char b = data_with_alpha[i * 4 + 2];
            unsigned char a = data_with_alpha[i * 4 + 3];

            if (a < 150) {
                // Transparent: set to white
                data[i * 3 + 0] = 255;
                data[i * 3 + 1] = 255;
                data[i * 3 + 2] = 255;
            }
            else {
                // Opaque: keep original color
                data[i * 3 + 0] = r;
                data[i * 3 + 1] = g;
                data[i * 3 + 2] = b;
            }
        }
    }

    int getWidth() { return this->width; }
    int getHeight() { return this->height; }
    int getChannels() { return this->channels; }
    unsigned char* getData() const { return this->data; }


    bool jpeg_compress(int quality)
    {
        this->applyAlphaMask();
        tjhandle compressor = tjInitCompress();
        if (!compressor) {
            std::cerr << "Failed to initialize TurboJPEG compressor\n";
            return false;
        }

        unsigned char* jpegBuf = nullptr;
        unsigned long jpegSize = 0;

        int ret = tjCompress2(
            compressor,
            data, width, 0, height, TJPF_RGB,
            &jpegBuf, &jpegSize,
            TJSAMP_444, quality, TJFLAG_FASTDCT
        );

        tjDestroy(compressor);

        if (ret != 0) {
            std::cerr << "JPEG compression failed: " << tjGetErrorStr() << std::endl;
            tjFree(jpegBuf);
            return false;
        }

        std::ofstream outFile(path_out, std::ios::binary);
        if (!outFile) {
            std::cerr << "Failed to open output file\n";
            tjFree(jpegBuf);
            return false;
        }

        outFile.write(reinterpret_cast<char*>(jpegBuf), jpegSize);
        outFile.close();
        tjFree(jpegBuf);

        std::cout << "Successfully compressed to " << path_out << "\n";
        return true;
    }
};

class JpgDecoder
{
private:
    const char* path_in;
    const char* path_out;
    int width, height, jpegSubsamp, jpegColorspace;
    std::vector<unsigned char> rgbBuffer;
public:

    JpgDecoder(const char* path_in, const char* path_out)
        : path_in(path_in), path_out(path_out)
    { }

    int getWidth() { return this->width; }
    int getHeight() { return this->height; }
    const unsigned char* getRGBData() const {
        return rgbBuffer.data();
    }
    bool jpeg_decompress()
    {
        std::ifstream inFile(path_in, std::ios::binary | std::ios::ate);
        if (!inFile) {
            std::cerr << "Failed to open input JPEG\n";
            return false;
        }

        std::streamsize jpegSize = inFile.tellg();
        inFile.seekg(0, std::ios::beg);

        std::vector<unsigned char> jpegBuf(jpegSize);
        if (!inFile.read(reinterpret_cast<char*>(jpegBuf.data()), jpegSize)) {
            std::cerr << "Failed to read JPEG data\n";
            return false;
        }

        tjhandle decompressor = tjInitDecompress();

        if (tjDecompressHeader3(decompressor, jpegBuf.data(), jpegSize, &width, &height, &jpegSubsamp, &jpegColorspace) != 0) {
            std::cerr << "Header read failed: " << tjGetErrorStr() << std::endl;
            tjDestroy(decompressor);
            return false;
        }

        rgbBuffer.resize(width * height * 3);

        if (tjDecompress2(
            decompressor,
            jpegBuf.data(), jpegSize,
            rgbBuffer.data(), width, 0, height,
            TJPF_RGB, TJFLAG_FASTDCT) != 0)
        {
            std::cerr << "Decompression failed: " << tjGetErrorStr() << std::endl;
            tjDestroy(decompressor);
            return false;
        }

        tjDestroy(decompressor);

        std::cout << "JPEG decompressed to RGB. Image size: " << width << "x" << height << "\n";

        return true;
    }
};

void JpegPSNRtoCSV(const std::string& imgPath,
                   const std::string& csvPath,
                   bool keepTempFiles = false,
                   const std::vector<int>& qualities = {0,5,10,20,30,40,50,60,70,80,90,100})
{
    if (qualities.empty())
        throw std::invalid_argument("quality list is empty");

    JpgEncoder reference(imgPath.c_str(), "dummy.jpg");  // output file unused
    reference.applyAlphaMask();                          // fills reference.getData()
    const unsigned char* refRGB = reference.getData();
    const int w = reference.getWidth();
    const int h = reference.getHeight();

    struct Row { int quality; double psnr; std::uintmax_t bytes; };
    std::vector<Row> rows;

    /* Iterate over quality levels                                     */
    for (int q : qualities)
    {
        if (q < 0 || q > 100) {
            std::cerr << "Skipping illegal quality " << q << '\n';
            continue;
        }

        /* a) Encode ---------------------------------------------------- */
        const std::string tmpName = "_psnr_q" + std::to_string(q) + ".jpg";
        JpgEncoder enc(imgPath.c_str(), tmpName.c_str());
        if (!enc.jpeg_compress(q)) {
            std::cerr << "Compression failed at quality " << q << '\n';
            continue;
        }

        /* b) Decode ---------------------------------------------------- */
        JpgDecoder dec(tmpName.c_str(), "unused.png");
        if (!dec.jpeg_decompress()) {
            std::cerr << "Decompression failed at quality " << q << '\n';
            continue;
        }

        if (dec.getWidth() != w || dec.getHeight() != h) {
            std::cerr << "Size mismatch at quality " << q << '\n';
            continue;
        }

        
        /* c) Compute PSNR --------------------------------------------- */
        const double psnr = computePSNR(refRGB, dec.getRGBData(), w, h);

        /* d) File-size (bytes) ---------------------------------------- */
        std::uintmax_t bytes = 0;
        try { bytes = std::filesystem::file_size(tmpName); }
        catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Warning: cannot stat \"" << tmpName << "\": " << e.what() << '\n';
        }

        rows.push_back({q, psnr, bytes});

        /* e) Clean-up temp file if desired ---------------------------- */
        if (!keepTempFiles) {
            std::error_code ec;
            std::filesystem::remove(tmpName, ec);
        }
    }

    /* ------------------------------------------------------------------ */
    /* 3) Write CSV                                                      */
    /* ------------------------------------------------------------------ */
    std::ofstream csv(csvPath, std::ios::trunc);
    if (!csv) throw std::runtime_error("cannot open CSV for writing");

    csv << "quality,psnr,size_bytes\n";
    csv << std::fixed << std::setprecision(6);
    for (const Row& r : rows)
        csv << r.quality << ',' << r.psnr << ',' << r.bytes << '\n';

    std::cout << "Wrote " << rows.size() << " rows to " << csvPath << '\n';
}
