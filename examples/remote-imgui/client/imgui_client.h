#pragma once

#include "../protocol.h"
#include "deserializer.h"
#include "network_client.h"

#include <memory>
#include <vector>
#include <iostream>

// Simplified version: use basic OpenGL headers
#include <thread>
#ifdef _WIN32
    #include <windows.h>
    #include <GL/gl.h>
#else
    #include <GL/gl.h>
#endif

namespace RemoteImGui {

class ImGuiClient {
public:
    ImGuiClient();
    ~ImGuiClient();

    bool initialize(int window_width, int window_height, const char* title);
    void cleanup();

    // Connection
    bool connect(const std::string& server_ip, int port);

    // Run
    void run();

    // Status query
    bool shouldClose() const;
    bool isConnected() const;

private:
    // Initialization related
    bool initGLFW();
    bool initOpenGL();
    bool setupImGui();
    void setupFontTexture();

    // Rendering related
    void renderFrame();
    void renderImGui();
    void processNetworkEvents();
#ifndef DISABLE_NETWORKING
    void renderDrawData(const FrameData* frame_data);
#else
    void renderDemoInterface();
#endif

    // Callback functions - reference imgui/examples/example_glfw_opengl3/main.cpp
    static void glfwErrorCallback(int error, const char* description);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void windowSizeCallback(GLFWwindow* window, int width, int height);
    static void charCallback(GLFWwindow* window, unsigned int codepoint);

    // Event sending
    void sendKeyEvent(int key, int action);
    void sendMouseEvent(int button, int action, double x, double y);
    void sendMouseMoveEvent(double x, double y);
    void sendScrollEvent(double xoffset, double yoffset);
    void sendResizeEvent(int width, int height);

#ifndef DISABLE_NETWORKING
    // Network client
    std::unique_ptr<NetworkClient> network_client_;
    std::unique_ptr<ImDrawDataDeserializer> deserializer_;
#endif

    // Member variables
    GLFWwindow* window_ = nullptr;
    int window_width_;
    int window_height_;
    bool initialized_ = false;
    bool connected_ = false;

    // ImGui related variables
    ImVec4 clear_color_ = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    GLuint font_texture_ = 0;
};

} // namespace RemoteImGui