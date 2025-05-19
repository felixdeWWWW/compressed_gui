#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h"

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <turbojpeg.h>
#include <cmath>
#include <cmath>

// Berechne PSNR zwischen zwei RGB-Bildern
double computePSNR(const unsigned char* original, const unsigned char* decoded, int width, int height) {
    double mse = 0.0;
    int totalPixels = width * height * 3;

    for (int i = 0; i < totalPixels; ++i) {
        int diff = static_cast<int>(original[i]) - static_cast<int>(decoded[i]);
        mse += diff * diff;
    }

    mse /= totalPixels;

    if (mse == 0) return INFINITY; 

    double psnr = 10.0 * std::log10((255.0 * 255.0) / mse);
    return psnr;
}

class Encoder 
{
private:
    const char* path_in;
    const char* path_out;
	int width, height, channels;
    unsigned char* data;
public:

    Encoder(const char* path_in, const char* path_out)
        : path_in(path_in), path_out(path_out)
    {
        data = stbi_load(path_in, &width, &height, &channels, 3);
        if (!data) {
            std::cerr << "Failed to load image\n";
            throw -1;
        }
        std::cout << "Image loaded: " << width << "x" << height << ", channels: " << 3 << std::endl;
    }

    ~Encoder() 
    {
        if (data) {
            stbi_image_free(data);
        }
    }

    int getWidth() { return this->width; }
    int getHeight() { return this->height; }
    int getChannels() { return this->channels; }
    unsigned char* getData() const { return this->data; }

	bool jpeg_compress(int quality) 
	{
        tjhandle compressor = tjInitCompress();
        if (!compressor) {
            std::cerr << "Failed to initialize TurboJPEG compressor\n";
            stbi_image_free(data);
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

class Decoder
{
private:
    const char* path_in;
    const char* path_out;
    int width, height, jpegSubsamp, jpegColorspace;
    std::vector<unsigned char> rgbBuffer;
public:

    Decoder(const char* path_in, const char* path_out)
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


double calculatePSNR (const unsigned char* original, const unsigned char* compressed, int width, int height, int channels, int depth) 
{
    double mse = 0.0;
    int max = 2 ^ depth;

    int size = width * height * channels;
    for (int i = 0; i < size; ++i) {
        int diff = original[i] - compressed[i];
        mse += diff * diff;
    }

    mse /= size;
    if (mse == 0) return INFINITY;

    double psnr = 10.0 * log10((max * max) / mse); 
    return psnr;
}

int main() 
{
    try
    {
        Encoder encoder("test.png", "output.jpg");

        if (!encoder.jpeg_compress(85)) {
            std::cerr << "Compression failed\n";
        }

        Decoder decoder("output.jpg", "egal.png");
        if (!decoder.jpeg_decompress()) {
            std::cerr << "Compression failed\n";
            return 1;
        }

        int w, h, c;
        unsigned char* originalData = stbi_load("test.png", &w, &h, &c, 3);
        if (!originalData) {
            std::cerr << "Failed to reload original image\n";
            return 1;
        }

        if (w != decoder.getWidth() || h != decoder.getHeight()) {
            std::cerr << "Image size mismatch!\n";
            stbi_image_free(originalData);
            return 1;
        }

        double psnr = computePSNR(originalData, decoder.getRGBData(), w, h);
        std::cout << "PSNR: " << psnr << " dB\n";

        stbi_image_free(originalData);

        std::cout << "encode and decode works!" << std::endl;
    }
    catch (int num)
    {
        std::cerr << "Reading image failed\n";
        return 1;
    }
    return 0;
}