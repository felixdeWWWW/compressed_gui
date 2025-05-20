// heic.h – header‑only HEIC encoder/decoder
#ifndef HEIC_H
#define HEIC_H

// --- Single‑header deps -----------------------------------------------------
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <libheif/heif.h>

#include <string>
#include <cstring>

class HeicEncoder {
public:
    explicit HeicEncoder(std::string input_path, std::string output_path = "")
        : input_path_(std::move(input_path)),
          output_path_(std::move(output_path)) {}

    bool encode(int quality = 90) const {
        int w, h, comp;
        unsigned char* data = stbi_load(input_path_.c_str(), &w, &h, &comp, 3);
        if (!data) return false;

        heif_context* ctx = heif_context_alloc();
        heif_image* img  = nullptr;
        heif_error err   = heif_image_create(w, h, heif_colorspace_RGB,
                                             heif_chroma_interleaved_RGB, &img);
        if (err.code) { stbi_image_free(data); heif_context_free(ctx); return false; }

        heif_image_add_plane(img, heif_channel_interleaved, w, h, 8);  // 3 interleaved RGB 8 bit channels
        uint8_t* dst = heif_image_get_plane(img, heif_channel_interleaved, nullptr);
        std::memcpy(dst, data, size_t(w * 3) * h);  // stride - w * 3 channels (bytes per row) for interleaved RGBRGBRGB...
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

    bool decode() const {
        heif_context* ctx = heif_context_alloc();
        heif_error err   = heif_context_read_from_file(ctx, input_path_.c_str(), nullptr);
        if (err.code) { heif_context_free(ctx); return false; }

        heif_image_handle* handle;
        err = heif_context_get_primary_image_handle(ctx, &handle);
        if (err.code) { heif_context_free(ctx); return false; }

        heif_image* img;
        err = heif_decode_image(handle, &img, heif_colorspace_RGB,
                                heif_chroma_interleaved_RGB, nullptr);
        heif_image_handle_release(handle);
        if (err.code) { heif_context_free(ctx); return false; }

        const int w = heif_image_get_width(img, heif_channel_interleaved);
        const int h = heif_image_get_height(img, heif_channel_interleaved);
        const uint8_t* src = heif_image_get_plane_readonly(img, heif_channel_interleaved, nullptr);

        const std::string out_path = output_path_.empty() ? default_out_path(".png") : output_path_;
        const int ok = stbi_write_png(out_path.c_str(), w, h, 3, src, w*3);

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

#endif // HEIC_H
