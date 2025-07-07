// gui.h – header‑only ImGui / GLFW frontend for HEIC & JPEG encode/decode in separate windows
#ifndef GUI_H
#define GUI_H

#include "heic.h"
#include "jpg.h"

#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <filesystem>
#include <cstring>
#include <vector>
#include <string>
#include <cstdio>

// ---------------------------------------------------------------------------
static void glfw_error_callback(int error, const char *desc)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, desc);
}

// ---------------------------------------------------------------------------
// Utility: derive a sibling file path with a new extension
static std::string change_extension(const char *path, const char *new_ext)
{
    std::filesystem::path p(path);
    p.replace_extension(new_ext);
    return p.string();
}

// ---------------------------------------------------------------------------
inline int run_gui()
{
    // Init GLFW + GL context
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    const char *glsl_version = "#version 130";
    GLFWwindow *window = glfwCreateWindow(960, 600, "Image Codec Demo", nullptr, nullptr);
    if (!window)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // ------ UI state ----------
    // HEIC
    char heic_in[512] = "";
    char heic_out[512] = "";
    int heic_quality = 90;
    bool heic_encode = true;
    bool heic_done_ok = false;
    bool heic_show_msg = false; // whether to show the green banner

    // JPEG
    char jpg_in[512] = "";
    char jpg_out[512] = "";
    int jpg_quality = 90;
    bool jpg_encode = true;
    bool jpg_done_ok = false;
    bool jpg_show_msg = false;

    // PSNR sweep
    char psnr_img[512] = "";
    char psnr_csv[512] = "";
    bool psnr_is_jpeg = true;
    bool psnr_done_ok = false;
    bool keep_tmp_files = false;
    bool psnr_show_msg = false;

    // Allow user to reposition the three windows manually, but start them side‑by‑side
    bool first_frame = true;

    // ------ Main loop ---------
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ---------------- HEIC WINDOW ----------------
        if (first_frame)
        {
            ImGui::SetNextWindowPos(ImVec2(10, 10));
            ImGui::SetNextWindowSize(ImVec2(300, 240));
        }
        ImGui::Begin("HEIC Compressor / Decompressor");

        ImGui::InputText("Input Path", heic_in, IM_ARRAYSIZE(heic_in));
        ImGui::InputText("Output Path", heic_out, IM_ARRAYSIZE(heic_out));

        ImGui::Checkbox("Encode (uncheck = Decode)", &heic_encode);
        if (heic_encode)
            ImGui::SliderInt("Quality", &heic_quality, 1, 100);

        if (ImGui::Button(heic_encode ? "Encode" : "Decode"))
        {
            heic_show_msg = false;
            heic_done_ok = false;
            if (std::strlen(heic_in) != 0)
            {
                // If output not provided, derive it
                if (std::strlen(heic_out) == 0)
                {
                    std::string def = change_extension(heic_in, heic_encode ? ".heic" : ".png");
                    std::strncpy(heic_out, def.c_str(), sizeof(heic_out));
                }
                try
                {
                    if (heic_encode)
                    {
                        HeicEncoder enc(heic_in, heic_out); // updated constructor with output_path
                        heic_done_ok = enc.encode(heic_quality);
                    }
                    else
                    {
                        HeicDecoder dec(heic_in, heic_out); // updated constructor with output_path
                        heic_done_ok = dec.decode();
                    }
                }
                catch (...)
                {
                    heic_done_ok = false;
                }
            }
            heic_show_msg = heic_done_ok;
        }

        if (heic_show_msg)
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Success! Saved to %s", heic_out);
        ImGui::End();

        // ---------------- JPEG WINDOW ----------------
        if (first_frame)
        {
            ImGui::SetNextWindowPos(ImVec2(330, 10));
            ImGui::SetNextWindowSize(ImVec2(300, 240));
        }
        ImGui::Begin("JPEG Compressor / Decompressor");

        ImGui::InputText("Input Path", jpg_in, IM_ARRAYSIZE(jpg_in));
        ImGui::InputText("Output Path", jpg_out, IM_ARRAYSIZE(jpg_out));

        ImGui::Checkbox("Encode (uncheck = Decode)", &jpg_encode);
        if (jpg_encode)
            ImGui::SliderInt("Quality", &jpg_quality, 1, 100);

        if (ImGui::Button(jpg_encode ? "Encode" : "Decode"))
        {
            jpg_show_msg = false;
            jpg_done_ok = false;
            if (std::strlen(jpg_in) != 0)
            {
                if (std::strlen(jpg_out) == 0)
                {
                    std::string def = change_extension(jpg_in, jpg_encode ? ".jpg" : ".png");
                    std::strncpy(jpg_out, def.c_str(), sizeof(jpg_out));
                }
                try
                {
                    if (jpg_encode)
                    {
                        JpgEncoder enc(jpg_in, jpg_out);
                        jpg_done_ok = enc.jpeg_compress(jpg_quality);
                    }
                    else
                    {
                        JpgDecoder dec(jpg_in, jpg_out);
                        jpg_done_ok = dec.jpeg_decompress();
                    }
                }
                catch (...)
                {
                    jpg_done_ok = false;
                }
            }
            jpg_show_msg = jpg_done_ok;
        }

        if (jpg_show_msg)
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Success! Saved to %s", jpg_out);

        ImGui::End();

        // ---------------- PSNR WINDOW ----------------
        if (first_frame)
        {
            ImGui::SetNextWindowPos(ImVec2(640, 10));
            ImGui::SetNextWindowSize(ImVec2(300, 240));
        }
        ImGui::Begin("PSNR Sweep to CSV");

        ImGui::InputText("Image Path", psnr_img, IM_ARRAYSIZE(psnr_img));
        ImGui::InputText("CSV Output", psnr_csv, IM_ARRAYSIZE(psnr_csv));

        ImGui::Text("Codec");
        ImGui::SameLine();
        if (ImGui::RadioButton("JPEG", psnr_is_jpeg))
            psnr_is_jpeg = true;
        ImGui::SameLine();
        if (ImGui::RadioButton("HEIC", !psnr_is_jpeg))
            psnr_is_jpeg = false;
        ImGui::Checkbox("Keep temp files", &keep_tmp_files);

        if (ImGui::Button("Run Sweep"))
        {
            psnr_show_msg = false;
            psnr_done_ok = false;
            if (std::strlen(psnr_img) != 0)
            {
                if (std::strlen(psnr_csv) == 0)
                {
                    std::string def = change_extension(psnr_img, psnr_is_jpeg ? "_jpeg_psnr.csv" : "_heic_psnr.csv");
                    std::strncpy(psnr_csv, def.c_str(), sizeof(psnr_csv));
                }
                try
                {
                    if (psnr_is_jpeg)
                        JpegPSNRtoCSV(psnr_img, psnr_csv, keep_tmp_files);
                    else
                        evaluateHeicQualitySweep(psnr_img, psnr_csv, keep_tmp_files);
                    psnr_done_ok = true;
                }
                catch (...)
                {
                    psnr_done_ok = false;
                }
            }
            psnr_show_msg = psnr_done_ok;
        }

        if (psnr_show_msg)
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "CSV written to %s", psnr_csv);

        ImGui::End();

        first_frame = false;

        // Render
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

#endif // GUI_H
