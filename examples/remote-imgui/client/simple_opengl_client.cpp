// OpenGL-based client for remote ImGui with actual window rendering
// Based on Dear ImGui example_glfw_opengl3

#include <stdio.h>
#include <stdlib.h>

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
#include "deserializer.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

class SimpleOpenGLClient {
public:
    SimpleOpenGLClient();
    ~SimpleOpenGLClient();

    bool initialize(int window_width, int window_height, const char* title);
    void run();
    void cleanup();

    // Connect to server (without real networking for now)
    bool connect(const std::string& server_ip, int port);

private:
    // GLFW window and OpenGL setup
    bool setupGLFWWindow();
    bool setupOpenGL();

    // ImGui integration
    void setupImGui();

    // Rendering
    void renderFrame();
    void renderImGui();

    // Members
    GLFWwindow* window_ = nullptr;
    int window_width_ = 1280;
    int window_height_ = 720;
    std::string window_title_;
    bool initialized_ = false;
    const char* glsl_version_ = nullptr;

    // ImGui state
    ImVec4 clear_color_ = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Data reception
    std::vector<uint8_t> received_data_;
    ImDrawDataDeserializer deserializer_;

    // Window flags
    bool show_demo_window_ = false;
    bool show_another_window_ = false;
    bool show_network_window_ = true;
    bool show_data_window_ = true;
    bool show_remote_window_ = true;

    // Frame counter
    int frame_count_ = 0;
};

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

SimpleOpenGLClient::SimpleOpenGLClient() {
    window_ = nullptr;
    glsl_version_ = nullptr;
}

SimpleOpenGLClient::~SimpleOpenGLClient() {
    cleanup();
}

bool SimpleOpenGLClient::initialize(int window_width, int window_height, const char* title) {
    window_width_ = window_width;
    window_height_ = window_height;
    window_title_ = title;

    // Setup window and OpenGL
    if (!setupGLFWWindow()) {
        return false;
    }
    if (!setupOpenGL()) {
        return false;
    }

    // Setup ImGui
    setupImGui();

    initialized_ = true;
    std::cout << "OpenGL client initialized successfully" << std::endl;
    return true;
}

bool SimpleOpenGLClient::connect(const std::string& server_ip, int port) {
    std::cout << "Connecting to " << server_ip << ":" << port << " (demo mode)" << std::endl;

    // Simulate receiving some data
    received_data_.clear();
    for (int i = 0; i < 256; ++i) {
        received_data_.push_back(0x40 + (i % 64));
    }

    return true;
}

bool SimpleOpenGLClient::setupGLFWWindow() {
    // Setup GLFW
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    glsl_version_ = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    glsl_version_ = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    glsl_version_ = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    window_ = glfwCreateWindow(window_width_, window_height_, window_title_.c_str(), NULL, NULL);
    if (window_ == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        return false;
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // Enable vsync

    std::cout << "GLFW window created successfully" << std::endl;
    return true;
}

bool SimpleOpenGLClient::setupOpenGL() {
    // For this demo, we'll assume OpenGL setup is successful
    // Check OpenGL version
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    std::cout << "OpenGL Renderer: " << renderer << std::endl;
    std::cout << "OpenGL Version: " << version << std::endl;
    return true;
}

void SimpleOpenGLClient::cleanup() {
    if (window_) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window_);
        window_ = nullptr;
    }

    glfwTerminate();
    std::cout << "OpenGL client cleaned up" << std::endl;
}

void SimpleOpenGLClient::setupImGui() {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init(glsl_version_);

    // Load Fonts
    io.Fonts->AddFontDefault();

    std::cout << "ImGui setup completed with OpenGL3 backend" << std::endl;
}

void SimpleOpenGLClient::renderFrame() {
    if (!initialized_ || !window_) {
        std::cerr << "Client not properly initialized!" << std::endl;
        return;
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Render ImGui
    renderImGui();

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window_, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color_.x * clear_color_.w, clear_color_.y * clear_color_.w, clear_color_.z * clear_color_.w, clear_color_.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Swap buffers
    glfwSwapBuffers(window_);
}

void SimpleOpenGLClient::renderImGui() {
    // Main menu bar
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New")) {}
            if (ImGui::MenuItem("Open", "Ctrl+O")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4")) {
                glfwSetWindowShouldClose(window_, true);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Demo Window", NULL, &show_demo_window_);
            ImGui::MenuItem("Another Window", NULL, &show_another_window_);
            ImGui::MenuItem("Network Settings", NULL, &show_network_window_);
            ImGui::MenuItem("Data Viewer", NULL, &show_data_window_);
            ImGui::MenuItem("Remote ImGui", NULL, &show_remote_window_);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                show_demo_window_ = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    // Remote ImGui main window
    static bool show_remote_window = true;
    if (show_remote_window) {
        ImGui::Begin("Remote ImGui Client - Real OpenGL Rendering", &show_remote_window);

        ImGui::Text("This is a real OpenGL-based ImGui client");
        ImGui::Text("Window size: %dx%d", window_width_, window_height_);

        // Get current window size
        int display_w, display_h;
        glfwGetWindowSize(window_, &display_w, &display_h);
        ImGui::Text("Current window size: %dx%d", display_w, display_h);

        ImGui::Separator();

        ImGui::ColorEdit3("Background Color", (float*)&clear_color_);

        ImGui::Text("Data reception:");
        ImGui::SameLine();
        ImGui::Text("Received: %zu bytes", received_data_.size());

        ImGui::Separator();

        ImGui::Text("Performance:");
        ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                    1000.0f / io.Framerate, io.Framerate);
        ImGui::Text("Vertices: %d, Indices: %d", io.MetricsRenderVertices, io.MetricsRenderIndices);

        if (frame_count_ % 60 == 0 && frame_count_ > 0) {
            // Print draw data info every 60 frames
            ImDrawData* draw_data = ImGui::GetDrawData();
            if (draw_data && draw_data->Valid) {
                std::cout << "Frame " << frame_count_ << ": "
                          << draw_data->CmdListsCount << " command lists, "
                          << draw_data->TotalVtxCount << " vertices, "
                          << draw_data->TotalIdxCount << " indices" << std::endl;
            }
        }

        ImGui::Separator();

        // Add some interactive elements
        static float rotation = 0.0f;
        ImGui::SliderFloat("Rotation", &rotation, 0.0f, 360.0f);
        rotation += 0.1f;
        if (rotation > 360.0f) rotation = 0.0f;

        static bool show_about = false;
        if (ImGui::Button("Show About")) {
            show_about = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Exit")) {
            glfwSetWindowShouldClose(window_, true);
        }

        if (show_about) {
            ImGui::Begin("About", &show_about);
            ImGui::Text("Remote ImGui Client");
            ImGui::Text("Built with real OpenGL rendering");
            ImGui::Text("Based on Dear ImGui official examples");
            ImGui::Separator();
            ImGui::Text("Controls:");
            ImGui::BulletText("Use menu to toggle windows");
            ImGui::BulletText("Click and drag to interact");
            ImGui::BulletText("Close window to exit");
            ImGui::End();
        }

        ImGui::End();
    }

    // Network settings window
    if (show_network_window_) {
        ImGui::Begin("Network Settings", &show_network_window_);

        static char server_ip[256] = "127.0.0.1";
        static int server_port = 8080;

        ImGui::InputText("Server IP", server_ip, sizeof(server_ip));
        ImGui::InputInt("Server Port", &server_port);

        if (ImGui::Button("Connect")) {
            connect(server_ip, server_port);
        }
        ImGui::SameLine();
        if (ImGui::Button("Disconnect")) {
            std::cout << "Disconnecting from server" << std::endl;
            received_data_.clear();
        }

        ImGui::Separator();

        if (ImGui::Button("Simulate Data Reception")) {
            // Simulate receiving different data patterns
            static int pattern = 0;
            received_data_.clear();
            for (int i = 0; i < 512; ++i) {
                received_data_.push_back(0x20 + ((i + pattern) % 96));
            }
            pattern = (pattern + 1) % 4;
            std::cout << "Simulated data reception: " << received_data_.size() << " bytes" << std::endl;
        }

        ImGui::Text("Note: This is a demo - no real networking implemented yet");

        ImGui::End();
    }

    // Data viewer window
    if (show_data_window_) {
        ImGui::Begin("Data Viewer", &show_data_window_);

        ImGui::Text("Received Data Display:");

        // Display received data in a hex viewer
        if (!received_data_.empty()) {
            if (ImGui::BeginChild("Hex View", ImVec2(0, 200), true)) {
                ImGui::Text("Hex view (first 256 bytes):");

                for (size_t i = 0; i < std::min((size_t)256, received_data_.size()); i += 16) {
                    // Offset
                    ImGui::Text("%04zx: ", i);
                    ImGui::SameLine();

                    // Hex bytes
                    for (int j = 0; j < 16 && (i + j) < received_data_.size(); ++j) {
                        ImGui::Text("%02X ", received_data_[i + j]);
                        ImGui::SameLine();
                    }

                    // Fill remaining space
                    for (int j = (i + 15 < received_data_.size() ? 16 : (received_data_.size() - i)); j < 16; ++j) {
                        ImGui::Text("   ");
                        ImGui::SameLine();
                    }

                    // ASCII representation
                    ImGui::SameLine();
                    ImGui::Text("| ");
                    for (int j = 0; j < 16 && (i + j) < received_data_.size(); ++j) {
                        char c = received_data_[i + j];
                        ImGui::Text("%c", (c >= 32 && c <= 126) ? c : '.');
                        ImGui::SameLine();
                    }
                    ImGui::NewLine();
                }

                ImGui::Text("Total: %zu bytes", received_data_.size());
                ImGui::EndChild();
            }
        } else {
            ImGui::Text("No data received yet");
        }

        ImGui::End();
    }

    // Demo window
    if (show_demo_window_) {
        ImGui::Begin("Demo Window", &show_demo_window_);
        ImGui::Text("This would show ImGui demo content");
        ImGui::Text("Current frame: %d", frame_count_);

        static float f = 0.0f;
        static int counter = 0;
        ImGui::SliderFloat("Float", &f, 0.0f, 1.0f);
        if (ImGui::Button("Button")) {
            counter++;
        }
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::End();
    }

    // Another window
    if (show_another_window_) {
        ImGui::Begin("Another Window", &show_another_window_);
        ImGui::Text("This is another demo window");
        ImGui::Text("You can add your own ImGui code here");

        static float f = 0.0f;
        static int counter = 0;
        ImGui::SliderFloat("Float", &f, 0.0f, 1.0f);
        if (ImGui::Button("Button")) {
            counter++;
        }
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        // Color picker
        static ImVec4 color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
        ImGui::ColorEdit3("Color", (float*)&color);

        ImGui::End();
    }

    // Control window
    {
        ImGui::Begin("Controls");

        ImGui::Checkbox("Demo Window", &show_demo_window_);
        ImGui::Checkbox("Another Window", &show_another_window_);
        ImGui::Checkbox("Network Settings", &show_network_window_);
        ImGui::Checkbox("Data Viewer", &show_data_window_);
        ImGui::Checkbox("Remote ImGui", &show_remote_window_);

        ImGui::Separator();

        ImGui::Text("Frame: %d", frame_count_);

        if (ImGui::Button("Exit Application")) {
            glfwSetWindowShouldClose(window_, true);
        }

        ImGui::Text("Close window to exit");

        ImGui::End();
    }

    // Increment frame counter
    frame_count_++;
}

void SimpleOpenGLClient::run() {
    if (!initialized_ || !window_) {
        std::cerr << "Client not initialized!" << std::endl;
        return;
    }

    std::cout << "Starting real OpenGL ImGui client..." << std::endl;
    std::cout << "Window created - you should see ImGui interface" << std::endl;
    std::cout << "Close window to quit" << std::endl;

    // Simulate initial connection
    connect("127.0.0.1", 8080);

    // Main loop
    while (!glfwWindowShouldClose(window_)) {
        // Poll and handle events
        glfwPollEvents();

        // Render frame
        renderFrame();
    }

    std::cout << "Real OpenGL client main loop ended" << std::endl;
}

int main(int, char** argv) {
    SimpleOpenGLClient client;

    // Initialize client
    if (!client.initialize(1280, 720, "Remote ImGui Client - Real OpenGL Rendering")) {
        std::cerr << "Failed to initialize real OpenGL client!" << std::endl;
        return -1;
    }

    // Run main loop
    client.run();

    return 0;
}