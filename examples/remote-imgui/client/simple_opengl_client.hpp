// OpenGL-based client for remote ImGui with actual window rendering
// Based on Dear ImGui example_glfw_opengl3

#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <vector>

// Windows OpenGL headers first
#ifdef _WIN32
#include <windows.h>
#include <GL/gl.h>
#endif

// GLFW headers
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>


// ImGui headers after OpenGL headers
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// Include remote_debugger after ImGui to ensure all types are properly defined
#include "network_client.hpp"
#include "remote_debugger.hpp"

// Using the remote_debugger library which includes all necessary components
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

class SimpleOpenGLClient {
public:
    SimpleOpenGLClient();
    ~SimpleOpenGLClient();

    bool Initialize(int window_width, int window_height, const char* title);
    void Run();
    void Cleanup();

    // Connect to server (without real networking for now)
    bool Connect(const std::string& server_ip, int port);
private:
    bool InitializeNetwork();

    // GLFW window and OpenGL setup
    bool SetupGLFWWindow();
    bool SetupOpenGL();

    // ImGui integration
    void SetupImGui();

    // Rendering
    void RenderFrame();
    void RenderLocalUI();
    void RenderServerLikeUI();
    void RenderRemoteFrameOnly();

    // Font texture management
    void UpdateFontTexture(const uint8_t* font_data, int width, int height);
    void CleanupFontTexture();

    // Input handling
    void SetupInputCallbacks();
    void CaptureInputEvents();
    void SendInputEvents();

    // Input event conversion helpers
    spatial::debugger::MouseButton GetImGuiMouseButton(int glfw_button);
    uint32_t GetImGuiKeyCode(int glfw_key);

    std::vector<uint8_t> ConvertAlphaToRgbaWhite(const uint8_t* alpha_data, int width, int height);
private:
    // Members
    GLFWwindow* window_ = nullptr;
    int window_width_ = 1280;
    int window_height_ = 720;
    std::string window_title_;
    bool initialized_ = false;
    const char* glsl_version_ = nullptr;

    // ImGui state
    ImVec4 clear_color_ = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Font texture
    GLuint font_texture_id_ = 0;

    // Data reception
    spatial::debugger::NetPacketBuffer cached_packet_;
    //std::vector<uint8_t> received_data_;
    spatial::debugger::ImDrawDataDeserializer deserializer_;

    // Network client
    std::unique_ptr<NetworkClient> network_client_;

    // Network processor for packet handling
    std::unique_ptr<spatial::debugger::NetPacketDispatcher> network_processor_;

    // Individual input events handling
    std::vector<spatial::debugger::MouseMoveEvent> mouse_move_events_;
    std::vector<spatial::debugger::MouseButtonEvent> mouse_button_events_;
    std::vector<spatial::debugger::MouseWheelEvent> mouse_wheel_events_;
    std::vector<spatial::debugger::KeyboardEvent> keyboard_events_;
    std::vector<spatial::debugger::CharEvent> char_events_;

    bool input_callbacks_setup_ = false;
    double last_mouse_x_ = 0.0;
    double last_mouse_y_ = 0.0;

    // Window flags - only show remote content and network settings
    bool show_network_window_ = true;

    // Frame counter
    int frame_count_ = 0;
};


