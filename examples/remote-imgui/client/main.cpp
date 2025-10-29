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
#include "network_client_impl.h"

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
    void integrateRemoteFrameData(ImDrawData* draw_data, const FrameData* frame_data);

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

    // Network client
    std::unique_ptr<NetworkClient> network_client_;

    // Window flags - only show remote content and network settings
    bool show_network_window_ = true;

    // Frame counter
    int frame_count_ = 0;
};

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

SimpleOpenGLClient::SimpleOpenGLClient() {
    window_ = nullptr;
    glsl_version_ = nullptr;

#ifdef _WIN32
    // Initialize Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed with error code: " << result << std::endl;
    }
#endif
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
    std::cout << "Connecting to " << server_ip << ":" << port << std::endl;

    // Create network client if not exists
    if (!network_client_) {
        network_client_ = std::make_unique<NetworkClient>();

        // Set network callbacks
        network_client_->setConnectCallback([this](bool connected) {
            if (connected) {
                std::cout << "Successfully connected to server" << std::endl;
            } else {
                std::cout << "Failed to connect to server" << std::endl;
            }
        });

        network_client_->setDisconnectCallback([this]() {
            std::cout << "Disconnected from server" << std::endl;
        });

        network_client_->setReceiveCallback([this](const uint8_t* data, size_t size) {
            // Process received ImGui draw data
            if (deserializer_.deserializePacket(data, size)) {
                std::cout << "Received and deserialized " << size << " bytes of draw data" << std::endl;

                // Store data for visualization
                received_data_.clear();
                received_data_.insert(received_data_.end(), data, data + size);
            } else {
                std::cout << "Failed to deserialize received data" << std::endl;
            }
        });
    }

    // Try to connect
    return network_client_->connect(server_ip, port);
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
    // Cleanup network client
    if (network_client_) {
        network_client_->disconnect();
        network_client_.reset();
    }

    if (window_) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window_);
        window_ = nullptr;
    }

    glfwTerminate();

#ifdef _WIN32
    // Cleanup Winsock
    WSACleanup();
#endif

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

    // Render ImGui (either remote or local UI)
    renderImGui();

    // Rendering
    ImDrawData* draw_data = ImGui::GetDrawData();

    // If we have remote frame data, integrate it into the draw data
    if (deserializer_.hasValidFrame()) {
        const FrameData* frame_data = deserializer_.getFrameData();
        if (frame_data) {
            integrateRemoteFrameData(draw_data, frame_data);
        }
    }

    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window_, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color_.x * clear_color_.w, clear_color_.y * clear_color_.w, clear_color_.z * clear_color_.w, clear_color_.w);
    glClear(GL_COLOR_BUFFER_BIT);

    if (draw_data != nullptr) {
      ImGui_ImplOpenGL3_RenderDrawData(draw_data);
    }

    // Swap buffers
    glfwSwapBuffers(window_);
}

void SimpleOpenGLClient::renderImGui() {
    // Only show local UI (network settings) when no remote data
    if (!deserializer_.hasValidFrame()) {
        ImGui::Text("Waiting for data from server...");
        ImGui::Text("Connect to a server to receive ImGui content");
    }
    // When we have remote data, local UI rendering will be handled in integrateRemoteFrameData

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
            if (network_client_) {
                network_client_->disconnect();
                std::cout << "Disconnecting from server" << std::endl;
            }
            received_data_.clear();
        }

        ImGui::Separator();

        // Show connection status
        if (network_client_) {
            bool connected = network_client_->isConnected();
            ImGui::Text("Status: %s", connected ? "Connected" : "Disconnected");
            ImGui::Text("Server: %s", network_client_->getServerInfo().c_str());
        } else {
            ImGui::Text("Status: Not initialized");
        }

        ImGui::Separator();

        ImGui::Text("Network functionality implemented with libevent");
        ImGui::Text("Data received: %zu bytes", received_data_.size());

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

    std::cout << "Starting remote ImGui client..." << std::endl;
    std::cout << "Window created - connect to server to receive content" << std::endl;

    // Simulate initial connection
    connect("127.0.0.1", 8080);

    // Main loop
    while (!glfwWindowShouldClose(window_)) {
        // Poll and handle events
        glfwPollEvents();

        // Process network events
        if (network_client_) {
            network_client_->processEvents();
        }

        // Render frame
        renderFrame();
    }

    std::cout << "Remote ImGui client ended" << std::endl;
}

void SimpleOpenGLClient::integrateRemoteFrameData(ImDrawData* draw_data, const FrameData* frame_data) {
    if (!draw_data || !frame_data || frame_data->header.cmd_lists_count == 0) {
        return;
    }

    // Update display size from server data
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(frame_data->header.display_size[0], frame_data->header.display_size[1]);
    io.DisplayFramebufferScale = ImVec2(frame_data->header.framebuffer_scale[0], frame_data->header.framebuffer_scale[1]);

    // Set display position from server data
    draw_data->DisplayPos = ImVec2(frame_data->header.display_pos[0], frame_data->header.display_pos[1]);
    draw_data->DisplaySize = ImVec2(frame_data->header.display_size[0], frame_data->header.display_size[1]);
    draw_data->FramebufferScale = ImVec2(frame_data->header.framebuffer_scale[0], frame_data->header.framebuffer_scale[1]);

    // Count total vertices and indices needed
    size_t total_vtx = 0;
    size_t total_idx = 0;
    for (uint32_t i = 0; i < frame_data->header.cmd_lists_count; i++) {
        if (i < frame_data->command_counts.size()) {
            uint32_t cmd_count = frame_data->command_counts[i];
            uint32_t command_offset = (i < frame_data->command_offsets.size()) ? frame_data->command_offsets[i] : 0;

            for (uint32_t j = 0; j < cmd_count && command_offset + j < frame_data->draw_commands.size(); j++) {
                total_idx += frame_data->draw_commands[command_offset + j].idx_count;
            }
        }
    }
    total_vtx = frame_data->vertex_buffers.size();

    if (total_vtx == 0 || total_idx == 0) {
        return;
    }

    // Allocate draw lists for remote content
    int old_cmd_lists_count = draw_data->CmdListsCount;
    draw_data->CmdListsCount += frame_data->header.cmd_lists_count;
    draw_data->CmdLists = (ImDrawList**)realloc(draw_data->CmdLists, draw_data->CmdListsCount * sizeof(ImDrawList*));

    // Create and populate each draw list from remote data
    size_t vertex_offset = 0;
    size_t index_offset = 0;
    size_t command_offset = 0;

    for (uint32_t i = 0; i < frame_data->header.cmd_lists_count; i++) {
        // Get offsets for this command list
        if (i < frame_data->vertex_offsets.size() &&
            i < frame_data->index_offsets.size() &&
            i < frame_data->command_offsets.size()) {

            vertex_offset = frame_data->vertex_offsets[i];
            index_offset = frame_data->index_offsets[i];
            command_offset = frame_data->command_offsets[i];
        }

        // Calculate command count for this list
        uint32_t cmd_count = (i < frame_data->command_counts.size()) ?
                           frame_data->command_counts[i] : 0;

        // Count vertices and indices for this list
        size_t list_vtx_count = 0;
        size_t list_idx_count = 0;

        for (uint32_t j = 0; j < cmd_count && command_offset + j < frame_data->draw_commands.size(); j++) {
            const DrawCmd& cmd = frame_data->draw_commands[command_offset + j];
            list_idx_count += cmd.idx_count;
        }

        // Determine vertex count range for this list
        size_t next_vertex_offset = (i + 1 < frame_data->vertex_offsets.size()) ?
                                  frame_data->vertex_offsets[i + 1] : frame_data->vertex_buffers.size();
        size_t next_index_offset = (i + 1 < frame_data->index_offsets.size()) ?
                                  frame_data->index_offsets[i + 1] : frame_data->index_buffers.size();

        list_vtx_count = next_vertex_offset - vertex_offset;
        list_idx_count = next_index_offset - index_offset;

        if (list_vtx_count == 0 || list_idx_count == 0) {
            continue;
        }

        // Create new draw list
        ImDrawList* new_list = new ImDrawList(ImGui::GetDrawListSharedData());

        // Allocate buffers
        new_list->IdxBuffer.resize(list_idx_count);
        new_list->VtxBuffer.resize(list_vtx_count);

        // Copy vertex data
        for (size_t v = 0; v < list_vtx_count; v++) {
            if (vertex_offset + v < frame_data->vertex_buffers.size()) {
                new_list->VtxBuffer[v] = frame_data->vertex_buffers[vertex_offset + v];
            }
        }

        // Copy index data (adjust indices to be relative to this list)
        for (size_t idx = 0; idx < list_idx_count; idx++) {
            if (index_offset + idx < frame_data->index_buffers.size()) {
                new_list->IdxBuffer[idx] = frame_data->index_buffers[index_offset + idx];
            }
        }

        // Copy draw commands
        new_list->CmdBuffer.resize(cmd_count);
        for (uint32_t j = 0; j < cmd_count; j++) {
            if (command_offset + j < frame_data->draw_commands.size()) {
                const DrawCmd& src_cmd = frame_data->draw_commands[command_offset + j];
                ImDrawCmd& dst_cmd = new_list->CmdBuffer[j];

                dst_cmd.ElemCount = src_cmd.idx_count;
                dst_cmd.ClipRect.x = src_cmd.clip_rect[0] / 1000.0f;
                dst_cmd.ClipRect.y = src_cmd.clip_rect[1] / 1000.0f;
                dst_cmd.ClipRect.z = src_cmd.clip_rect[2] / 1000.0f;
                dst_cmd.ClipRect.w = src_cmd.clip_rect[3] / 1000.0f;
                dst_cmd.TextureId = (ImTextureID)(uintptr_t)src_cmd.texture_id;

                // Set index offset for this command within the list
                dst_cmd.IdxOffset = 0; // Will be calculated during rendering
                dst_cmd.VtxOffset = 0; // Will be calculated during rendering
            }
        }

        // Add the new draw list to draw data
        draw_data->CmdLists[old_cmd_lists_count + i] = new_list;
    }

    // Update total counts
    draw_data->TotalVtxCount += total_vtx;
    draw_data->TotalIdxCount += total_idx;
    draw_data->Valid = true;
}

int main(int, char** argv) {
    SimpleOpenGLClient client;

    // Initialize client
    if (!client.initialize(1280, 720, "Remote ImGui Client")) {
        std::cerr << "Failed to initialize remote ImGui client!" << std::endl;
        return -1;
    }

    // Run main loop
    client.run();

    return 0;
}