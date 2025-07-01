#pragma once
#include <cmath>

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