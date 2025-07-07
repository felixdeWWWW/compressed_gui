// heic_extended.h – extended header‑only HEIC encoder/decoder with quality sweep benchmark

#ifndef HEIC_EXTENDED_H
#define HEIC_EXTENDED_H

// --- Single-header dependencies ---------------------------------------------
#include <stb_image.h>           // Image loading (PNG, JPEG, etc.)
#include "stb_image_write.h"     // Image writing (PNG, BMP, etc.)
#include "helpers.h"             // Helper functions (e.g., PSNR calculation)

#include <libheif/heif.h>        // Main library for HEIC encoding/decoding

#include <string>
#include <cstring>
#include <vector>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <cmath>
#include <filesystem>            // C++17 for temp file handling and file size

// ----------------------------------------------------------------------------
// HEIC Encoder class
// Encodes an input image file to HEIC format at specified quality
// ----------------------------------------------------------------------------
class HeicEncoder {
public:
    // Constructor with input and optional output path
    explicit HeicEncoder(std::string input_path, std::string output_path = "")
        : input_path_(std::move(input_path)),
        output_path_(std::move(output_path)) {}

    // Encode the input image to HEIC at the specified quality (0–100).
    bool encode(int quality = 90) const {
        int w, h, comp;

        // Load the input image using stb_image (always convert to RGB)
        unsigned char* data = stbi_load(input_path_.c_str(), &w, &h, &comp, 3);
        if (!data) return false;

        // Allocate HEIF context and image container
        heif_context* ctx = heif_context_alloc();
        heif_image* img = nullptr;

        // Create a new HEIF image with RGB interleaved layout
        heif_error err = heif_image_create(w, h, heif_colorspace_RGB,
            heif_chroma_interleaved_RGB, &img);
        if (err.code) {
            stbi_image_free(data);
            heif_context_free(ctx);
            return false;
        }

        // Add interleaved RGB plane with 8 bits per channel
        heif_image_add_plane(img, heif_channel_interleaved, w, h, 8);
        uint8_t* dst = heif_image_get_plane(img, heif_channel_interleaved, nullptr);

        // Copy input data to HEIF image buffer
        std::memcpy(dst, data, size_t(w * 3) * h);
        stbi_image_free(data);  // Free original image memory

        // Set up HEVC encoder
        heif_encoder* enc;
        heif_context_get_encoder_for_format(ctx, heif_compression_HEVC, &enc);
        heif_encoder_set_lossy_quality(enc, quality);  // Set compression quality

        // Encode image and get handle
        heif_image_handle* handle;
        err = heif_context_encode_image(ctx, img, enc, nullptr, &handle);
        heif_encoder_release(enc);
        heif_image_release(img);
        if (err.code) {
            heif_context_free(ctx);
            return false;
        }

        // Determine output path if not provided
        const std::string out_path = output_path_.empty() ? default_out_path(".heic") : output_path_;

        // Write encoded image to file
        err = heif_context_write_to_file(ctx, out_path.c_str());
        heif_context_free(ctx);
        return err.code == 0;
    }

private:
    // Generates default output path based on input file and new extension
    std::string default_out_path(const char* ext) const {
        const size_t dot = input_path_.find_last_of('.');
        return input_path_.substr(0, dot == std::string::npos ? input_path_.size() : dot) + ext;
    }

    std::string input_path_;   // Source image path
    std::string output_path_;  // Output HEIC path (optional)
};

// ----------------------------------------------------------------------------
// HEIC Decoder class
// Decodes HEIC file to PNG using libheif and stb_image_write
// ----------------------------------------------------------------------------
class HeicDecoder {
public:
    // Constructor with input and optional output path
    explicit HeicDecoder(std::string input_path, std::string output_path = "")
        : input_path_(std::move(input_path)),
        output_path_(std::move(output_path)) {}

    // Decode HEIC image to PNG
    bool decode() const {
        // Load HEIC context from file
        heif_context* ctx = heif_context_alloc();
        heif_error err = heif_context_read_from_file(ctx, input_path_.c_str(), nullptr);
        if (err.code) {
            heif_context_free(ctx);
            fprintf(stderr, "Error reading from file %d\n", err.code);
            return false;
        }

        // Get handle to primary image
        heif_image_handle* handle;
        err = heif_context_get_primary_image_handle(ctx, &handle);
        if (err.code) {
            heif_context_free(ctx);
            fprintf(stderr, "Error getting image handle %d\n", err.code);
            return false;
        }

        // Decode to interleaved RGB
        heif_image* img;
        err = heif_decode_image(handle, &img, heif_colorspace_RGB,
            heif_chroma_interleaved_RGB, nullptr);
        heif_image_handle_release(handle);
        if (err.code) {
            heif_context_free(ctx);
            fprintf(stderr, "Error decoding image %d\n %s", err.code, err.message);
            return false;
        }

        // Get image dimensions and pixel data
        const int w = heif_image_get_width(img, heif_channel_interleaved);
        const int h = heif_image_get_height(img, heif_channel_interleaved);
        const uint8_t* src = heif_image_get_plane_readonly(img, heif_channel_interleaved, nullptr);

        // Determine output PNG path
        const std::string out_path = output_path_.empty() ? default_out_path(".png") : output_path_;

        // Write PNG using stb_image_write
        const int ok = stbi_write_png(out_path.c_str(), w, h, 3, src, w * 3);
        fprintf(stdout, "Reading done %d\n", ok);

        // Cleanup
        heif_image_release(img);
        heif_context_free(ctx);
        return ok != 0;
    }

private:
    // Generates default output path based on input file and new extension
    std::string default_out_path(const char* ext) const {
        const size_t dot = input_path_.find_last_of('.');
        return input_path_.substr(0, dot == std::string::npos ? input_path_.size() : dot) + ext;
    }

    std::string input_path_;   // Source HEIC file
    std::string output_path_;  // Output PNG path (optional)
};

// ----------------------------------------------------------------------------
// Result structure for HEIC quality sweep
// ----------------------------------------------------------------------------
struct HeicQualityResult {
    int quality;                 // Compression quality (0–100)
    double psnr;                 // Peak Signal-to-Noise Ratio in dB
    std::uintmax_t file_size;    // Output HEIC file size in bytes
};

// ----------------------------------------------------------------------------
// Evaluate compression quality sweep
// Encode image at multiple quality levels, decode, compare PSNR, and save CSV
// ----------------------------------------------------------------------------
inline bool evaluateHeicQualitySweep(
    const std::string& image_path,
    const std::string& csv_path = "heic_quality.csv",
    bool keep_temp_files = false,
    const std::vector<int>& qualities = {0, 5, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100})
{
    // ---------------------------------------------------------------------
    // 1. Load reference image (original)
    // ---------------------------------------------------------------------
    int ref_w, ref_h, ref_comp;
    unsigned char* reference =
        stbi_load(image_path.c_str(), &ref_w, &ref_h, &ref_comp, 3 /*force RGB*/);
    if (!reference) {
        std::cerr << "(HEIC SWEEP) Could not load reference image: " << image_path << '\n';
        return false;
    }

    // ---------------------------------------------------------------------
    // 2. Open results CSV
    // ---------------------------------------------------------------------
    std::ofstream csv(csv_path, std::ios::trunc);
    if (!csv) {
        std::cerr << "(HEIC SWEEP) Unable to open CSV for writing: " << csv_path << '\n';
        stbi_image_free(reference);
        return false;
    }
    csv << "quality,psnr,size_bytes\n";

    // ---------------------------------------------------------------------
    // 3. Decide where to place the intermediate HEIC / PNG files
    //    * When keep_temp_files == true  ➜ same directory as the source image
    //    * When keep_temp_files == false ➜ system temporary directory (and later removed)
    // ---------------------------------------------------------------------
    const std::filesystem::path img_path(image_path);
    const std::filesystem::path base_dir = keep_temp_files
                                           ? img_path.parent_path()
                                           : std::filesystem::temp_directory_path();
    const std::string stem = img_path.stem().string();

    // ---------------------------------------------------------------------
    // 4. Iterate over quality settings
    // ---------------------------------------------------------------------
    for (int q : qualities) {
        const std::filesystem::path encoded_path =
            base_dir / (stem + std::string("_q") + std::to_string(q) + ".heic");
        const std::filesystem::path decoded_path =
            base_dir / (stem + std::string("_q") + std::to_string(q) + ".png");

        // ---- Encode ------------------------------------------------------
        HeicEncoder encoder(image_path, encoded_path.string());
        if (!encoder.encode(q)) {
            std::cerr << "(HEIC SWEEP) Encoding failed at quality=" << q << '\n';
            continue;
        }

        // ---- Decode ------------------------------------------------------
        HeicDecoder decoder(encoded_path.string(), decoded_path.string());
        if (!decoder.decode()) {
            std::cerr << "(HEIC SWEEP) Decoding failed at quality=" << q << '\n';
            continue;
        }

        // ---- Load decoded PNG for PSNR ----------------------------------
        int dec_w, dec_h, dec_comp;
        unsigned char* decoded =
            stbi_load(decoded_path.string().c_str(), &dec_w, &dec_h, &dec_comp, 3);
        if (!decoded) {
            std::cerr << "(HEIC SWEEP) Failed to load decoded PNG at quality=" << q << '\n';
            continue;
        }

        if (dec_w != ref_w || dec_h != ref_h) {
            std::cerr << "(HEIC SWEEP) Dimension mismatch at quality=" << q << '\n';
            stbi_image_free(decoded);
            continue;
        }

        // ---- Metrics -----------------------------------------------------
        const double psnr_db = computePSNR(reference, decoded, ref_w, ref_h);
        const std::uintmax_t bytes = std::filesystem::file_size(encoded_path);

        csv << q << ',' << std::fixed << std::setprecision(4) << psnr_db << ',' << bytes << '\n';
        stbi_image_free(decoded);

        // ---- Clean‑up decoded PNG ---------------------------------------
        // PNG is only needed for quality measurement, never persisted
        std::error_code ec;
        std::filesystem::remove(decoded_path, ec);

        // ---- Optionally clean‑up HEIC ------------------------------------
        if (!keep_temp_files) {
            std::filesystem::remove(encoded_path, ec);
        }
    }

    // ---------------------------------------------------------------------
    // 5. Finalise
    // ---------------------------------------------------------------------
    stbi_image_free(reference);
    csv.close();

    std::cout << "(HEIC SWEEP) Results written to " << csv_path << '\n';
    if (keep_temp_files) {
        std::cout << "(HEIC SWEEP) Compressed HEIC files saved in " << base_dir << '\n';
    }

    return true;
}

#endif // HEIC_EXTENDED_H
