// gui.h – header‑only ImGui / GLFW frontend for HEIC encode/decode
#ifndef GUI_H
#define GUI_H

#include "heic.h"

#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

// ---------------------------------------------------------------------------
static void glfw_error_callback(int error, const char* desc)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, desc);
}

// ---------------------------------------------------------------------------
inline int run_gui()
{
    // Init GLFW + GL context
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return 1;

    const char* glsl_version = "#version 130";
    GLFWwindow* window = glfwCreateWindow(640, 480, "HEIC Demo", nullptr, nullptr);
    if (!window) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // ------ UI state ----------
    char path[512] = "";
    int  quality   = 90;
    bool encode    = true;   // encode or decode mode
    bool done_ok   = false;

    // ------ Main loop ---------
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("HEIC Tool");
        ImGui::InputText("Image path", path, IM_ARRAYSIZE(path));

        ImGui::Checkbox("Encode (uncheck = Decode)", &encode);
        if (encode)
            ImGui::SliderInt("Quality", &quality, 1, 100);

        if (ImGui::Button(encode ? "Encode" : "Decode"))
        {
            if (encode)
            {
                HeicEncoder enc(path);
                done_ok = enc.encode(quality);
            }
            else
            {
                HeicDecoder dec(path);
                done_ok = dec.decode();
            }
        }

        if (done_ok)
            ImGui::TextColored(ImVec4(0,1,0,1), "Success! Output saved next to original.");

        ImGui::End();

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
