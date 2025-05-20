#include "heic.h"
#include <iostream>

int main() 
{
    try
    {
        HeicEncoder encoder("123.png", "123.heic");

        if (!encoder.encode(85)) {
            std::cerr << "Compression failed\n";
        }

        HeicDecoder decoder("123.heic", "123.png");
        if (!decoder.decode()) {
            std::cerr << "Compression failed\n";
            return 1;
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