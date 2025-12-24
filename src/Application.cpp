#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Vendor/window.h"
#include "Renderer.h"
#include <vector>
#include <iostream>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <thread>

int main(void) {
    Window window(1280, 720, "Procedural Trees");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    
    ImGui_ImplGlfw_InitForOpenGL(window.GetWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 130");

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    Renderer renderer;
    renderer.Init();
    
    while (!window.shouldClose()) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        window.calculateDeltaTime(deltaTime, lastFrame);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderer.processKeyboardInput(window.GetWindow(), deltaTime);
        renderer.ProcessMouseInput(window.GetWindow(), deltaTime);
        renderer.Render();

        renderer.RenderDebugUI(deltaTime);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window.GetWindow());
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}