#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <libheif/heif.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <memory>

// OpenGL / ImGui / GLFW
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

// ------------------------
// Utility
// ------------------------
static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// ------------------------
// HEIC encoder class
// ------------------------
class HeicEncoder {
public:
    HeicEncoder(std::string input_path, std::string output_path)
        : input_path_(std::move(input_path)), output_path_(std::move(output_path)) {}

    // Encode the image to HEIC with the given quality (0‑100). Returns true on success.
    bool encode_heic(int quality = 90) const {
        int w, h, comp;
        unsigned char* data = stbi_load(input_path_.c_str(), &w, &h, &comp, 3);
        if (!data) {
            fprintf(stderr, "stb_image: %s\n", stbi_failure_reason());
            return false;
        }

        std::unique_ptr<heif_context, decltype(&heif_context_free)> ctx(heif_context_alloc(), &heif_context_free);

        heif_image* img = nullptr;
        heif_error err = heif_image_create(w, h, heif_colorspace_RGB, heif_chroma_interleaved_RGB, &img);
        if (err.code) {
            stbi_image_free(data);
            return false;
        }

        heif_image_add_plane(img, heif_channel_interleaved, w, h, 24);
        const int stride = w * 3;
        uint8_t* dst = heif_image_get_plane(img, heif_channel_interleaved, nullptr);
        memcpy(dst, data, static_cast<size_t>(stride) * h);
        stbi_image_free(data);

        heif_encoder* encoder;
        err = heif_context_get_encoder_for_format(ctx.get(), heif_compression_HEVC, &encoder);
        if (err.code) {
            heif_image_release(img);
            return false;
        }

        heif_encoder_set_lossy_quality(encoder, quality);

        heif_image_handle* handle;
        err = heif_context_encode_image(ctx.get(), img, encoder, nullptr, &handle);
        heif_encoder_release(encoder);
        heif_image_release(img);
        if (err.code)
            return false;

        err = heif_context_write_to_file(ctx.get(), output_path_.c_str());
        return err.code == 0;
    }

private:
    std::string input_path_;
    std::string output_path_;
};

// Derive an output path with .heic extension next to the input file
static std::string make_output_path(const char* in)
{
    std::string out(in);
    size_t dot = out.find_last_of('.');
    if (dot != std::string::npos)
        out.resize(dot);
    out += ".heic";
    return out;
}

// ------------------------
// GUI program entry point
// ------------------------
int main()
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    const char* glsl_version = "#version 130";
    GLFWwindow* window = glfwCreateWindow(640, 480, "HEIC Demo", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    char inPath[512] = "";
    int  quality     = 90;
    bool success     = false;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("HEIC Encoder");
        ImGui::InputText("Image path", inPath, IM_ARRAYSIZE(inPath));
        ImGui::SliderInt("Quality", &quality, 0, 100);
        if (ImGui::Button("Encode to HEIC")) {
            std::string outPath = make_output_path(inPath);
            HeicEncoder encoder(inPath, outPath);
            success = encoder.encode_heic(quality);
        }
        if (success)
            ImGui::TextColored(ImVec4(0.f, 1.f, 0.f, 1.f), "Done! Saved alongside original.");
        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}