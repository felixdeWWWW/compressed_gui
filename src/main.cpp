#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "logger.h"

#include <libheif/heif.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>
#include <cstdio>
#include <string>


static void glfw_error_callback(int error, const char* description)
{
    LOG_MSG(description);
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Encodes the given file to <same-name>.heic beside it
bool encode_to_heic(const char* input_path)
{
    int w, h, comp;
    unsigned char* data = stbi_load(input_path, &w, &h, &comp, 3);
    if (!data) {
        MessageBoxA(nullptr, "Failed to load image", "Error", MB_OK|MB_ICONERROR);
        LOG_MSG(stbi_failure_reason());
        return false;
    }
    

    heif_context* ctx = heif_context_alloc();
    heif_image* img  = nullptr;

    heif_error err = heif_image_create(w, h, heif_colorspace_RGB, heif_chroma_interleaved_RGB, &img);

    if (err.code) { 
        LOG_MSG("failed to create heif image");
        stbi_image_free(data);
        return false; }

    heif_image_add_plane(img, heif_channel_interleaved, w, h, 24);
    const int stride = w * 3;
    uint8_t* dst = heif_image_get_plane(img, heif_channel_interleaved, nullptr);
    memcpy(dst, data, size_t(stride) * h);
    stbi_image_free(data);

    heif_encoder* encoder;
    heif_context_get_encoder_for_format(ctx, heif_compression_HEVC, &encoder);
    heif_encoder_set_lossy_quality(encoder, 90); // 0‑100

    heif_image_handle* handle;
    err = heif_context_encode_image(ctx, img, encoder, nullptr, &handle);
    heif_encoder_release(encoder);
    heif_image_release(img);
    if (err.code) { heif_context_free(ctx); return false; }

    // build output path
    std::string out = input_path;
    size_t dot = out.find_last_of('.');
    if (dot != std::string::npos) out.resize(dot);
    out += ".heic";

    err = heif_context_write_to_file(ctx, out.c_str());
    heif_context_free(ctx);
    return err.code == 0;
}

int main()
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    const char* glsl_version = "#version 130";
    GLFWwindow* window = glfwCreateWindow(640, 480, "HEIC Demo", nullptr, nullptr);
    if (!window) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    char path[512] = "";
    bool success = false;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("HEIC Encoder");
        ImGui::InputText("Image path", path, IM_ARRAYSIZE(path));
        if (ImGui::Button("Encode to HEIC")) {
            success = encode_to_heic(path);
        }
        if (success) ImGui::TextColored(ImVec4(0,1,0,1), "Done! Saved alongside original.");
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