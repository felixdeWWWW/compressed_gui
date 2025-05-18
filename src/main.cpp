#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h"

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <turbojpeg.h> // needs to be set up

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
            throw - 1;
        }
        std::cout << "Image loaded: " << width << "x" << height << ", channels: " << 3 << std::endl;
    }

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

        stbi_image_free(data);
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
    unsigned char* data;
public:

    Decoder(const char* path_in, const char* path_out)
        : path_in(path_in), path_out(path_out)
    { }

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

        std::vector<unsigned char> rgbBuffer(width * height * 3);

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

int main() {
    try
    {
        Encoder encoder("test.png", "output.jpg");
        if (!encoder.jpeg_compress(85)) {
            std::cerr << "Compression failed\n";
        }
        Decoder decoder("output.jpg", "egal.png");
        if (!decoder.jpeg_decompress()) {
            std::cerr << "Compression failed\n";
        }
        std::cout << "encode and decode works!" << std::endl;
    }
    catch (int num)
    {
        std::cerr << "Reading image failed\n";
        return 1;
    }
    return 0;
}