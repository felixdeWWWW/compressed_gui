// heic_extended.h – extended header‑only HEIC encoder/decoder with quality sweep benchmark
#ifndef HEIC_EXTENDED_H
#define HEIC_EXTENDED_H

// --- Single‑header deps -----------------------------------------------------
#include <stb_image.h>
#include "stb_image_write.h"
#include "helpers.h"

#include <libheif/heif.h>

#include <string>
#include <cstring>
#include <vector>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <cmath>
#include <filesystem>          // C++17 for temp dir + file sizes


class HeicEncoder {
public:
    explicit HeicEncoder(std::string input_path, std::string output_path = "")
        : input_path_(std::move(input_path)),
          output_path_(std::move(output_path)) {}

    // Encode the input image to HEIC at the specified quality (0‑100).
    bool encode(int quality = 90) const {
        int w, h, comp;
        unsigned char* data = stbi_load(input_path_.c_str(), &w, &h, &comp, 3);
        if (!data) return false;

        heif_context* ctx = heif_context_alloc();
        heif_image* img  = nullptr;
        heif_error err   = heif_image_create(w, h, heif_colorspace_RGB,
                                             heif_chroma_interleaved_RGB, &img);
        if (err.code) { stbi_image_free(data); heif_context_free(ctx); return false; }

        heif_image_add_plane(img, heif_channel_interleaved, w, h, 8);  // 3 interleaved RGB 8‑bit channels
        uint8_t* dst = heif_image_get_plane(img, heif_channel_interleaved, nullptr);
        std::memcpy(dst, data, size_t(w * 3) * h);
        stbi_image_free(data);

        heif_encoder* enc;
        heif_context_get_encoder_for_format(ctx, heif_compression_HEVC, &enc);
        heif_encoder_set_lossy_quality(enc, quality);

        heif_image_handle* handle;
        err = heif_context_encode_image(ctx, img, enc, nullptr, &handle);
        heif_encoder_release(enc);
        heif_image_release(img);
        if (err.code) { heif_context_free(ctx); return false; }

        const std::string out_path = output_path_.empty() ? default_out_path(".heic") : output_path_;
        err = heif_context_write_to_file(ctx, out_path.c_str());
        heif_context_free(ctx);
        return err.code == 0;
    }

private:
    std::string default_out_path(const char* ext) const {
        const size_t dot = input_path_.find_last_of('.');
        return input_path_.substr(0, dot == std::string::npos ? input_path_.size() : dot) + ext;
    }
    std::string input_path_;
    std::string output_path_;
};


class HeicDecoder {
public:
    explicit HeicDecoder(std::string input_path, std::string output_path = "")
        : input_path_(std::move(input_path)),
          output_path_(std::move(output_path)) {}

    // Decode the HEIC to PNG. Returns true on success.
    bool decode() const {
        heif_context* ctx = heif_context_alloc();
        heif_error err   = heif_context_read_from_file(ctx, input_path_.c_str(), nullptr);
        if (err.code) { heif_context_free(ctx); fprintf(stderr, "Error reading from file %d\n", err.code); return false; }

        heif_image_handle* handle;
        err = heif_context_get_primary_image_handle(ctx, &handle);
        if (err.code) { heif_context_free(ctx); fprintf(stderr, "Error getting image handle %d\n", err.code); return false; }

        heif_image* img;
        err = heif_decode_image(handle, &img, heif_colorspace_RGB,
                                heif_chroma_interleaved_RGB, nullptr);
        heif_image_handle_release(handle);
        if (err.code) { heif_context_free(ctx); fprintf(stderr, "Error decoding image %d\n %s", err.code, err.message); return false; }

        const int w = heif_image_get_width(img, heif_channel_interleaved);
        const int h = heif_image_get_height(img, heif_channel_interleaved);
        const uint8_t* src = heif_image_get_plane_readonly(img, heif_channel_interleaved, nullptr);

        const std::string out_path = output_path_.empty() ? default_out_path(".png") : output_path_;
        const int ok = stbi_write_png(out_path.c_str(), w, h, 3, src, w*3);
        fprintf(stdout, "Reading done %d\n", ok);

        heif_image_release(img);
        heif_context_free(ctx);
        return ok != 0;
    }

private:
    std::string default_out_path(const char* ext) const {
        const size_t dot = input_path_.find_last_of('.');
        return input_path_.substr(0, dot == std::string::npos ? input_path_.size() : dot) + ext;
    }
    std::string input_path_;
    std::string output_path_;
};


// Stores a single row of benchmark output
struct HeicQualityResult {
    int quality;                 // Encoder quality parameter (0–100)
    double psnr;                 // Reconstructed PSNR (dB)
    std::uintmax_t file_size;    // Encoded HEIC file size in bytes
};

inline bool evaluateHeicQualitySweep(const std::string& image_path,
    const std::string& csv_path = "heic_quality.csv",
    bool keep_temp_files = false,
    const std::vector<int>& qualities = {0,5,10,20,30,40,50,60,70,80,90,100})
{
    // Load reference image once
    int ref_w, ref_h, ref_comp;
    unsigned char* reference = stbi_load(image_path.c_str(), &ref_w, &ref_h, &ref_comp, 3);
    if (!reference) {
        std::cerr << "(HEIC SWEEP) Could not load reference image: " << image_path << '\n';
        return false;
    }

    // Prepare CSV output
    std::ofstream csv(csv_path, std::ios::trunc);
    if (!csv) {
        std::cerr << "(HEIC SWEEP) Unable to open CSV for writing: " << csv_path << '\n';
        stbi_image_free(reference);
        return false;
    }
    csv << "quality,psnr,size_bytes\n";

    const auto temp_dir = std::filesystem::temp_directory_path();

    for (int q : qualities) {
        const std::string encoded_path = (temp_dir / ("heic_q" + std::to_string(q) + ".heic")).string();
        const std::string decoded_path = (temp_dir / ("heic_q" + std::to_string(q) + ".png")).string();

        HeicEncoder encoder(image_path, encoded_path);
        if (!encoder.encode(q)) {
            std::cerr << "(HEIC SWEEP) Encoding failed at quality=" << q << '\n';
            continue;  // Skip to next quality
        }

        HeicDecoder decoder(encoded_path, decoded_path);
        if (!decoder.decode()) {
            std::cerr << "(HEIC SWEEP) Decoding failed at quality=" << q << '\n';
            continue;
        }

        int dec_w, dec_h, dec_comp;
        unsigned char* decoded = stbi_load(decoded_path.c_str(), &dec_w, &dec_h, &dec_comp, 3);
        if (!decoded) {
            std::cerr << "(HEIC SWEEP) Failed to load decoded PNG at quality=" << q << '\n';
            continue;
        }
        if (dec_w != ref_w || dec_h != ref_h) {
            std::cerr << "(HEIC SWEEP) Dimension mismatch at quality=" << q << '\n';
            stbi_image_free(decoded);
            continue;
        }

        const double psnr_db = computePSNR(reference, decoded, ref_w, ref_h);
        const std::uintmax_t bytes = std::filesystem::file_size(encoded_path);

        csv << q << ',' << std::fixed << std::setprecision(4) << psnr_db << ',' << bytes << '\n';

        stbi_image_free(decoded);

        if (!keep_temp_files) {
            std::error_code ec;
            std::filesystem::remove(encoded_path, ec);
            std::filesystem::remove(decoded_path, ec);
        }
    }

    stbi_image_free(reference);
    csv.close();
    std::cout << "(HEIC SWEEP) Results written to " << csv_path << '\n';
    return true;
}

#endif // HEIC_EXTENDED_H
