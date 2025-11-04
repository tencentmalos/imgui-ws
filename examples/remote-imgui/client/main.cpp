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

// Include remote_debugger after ImGui to ensure all types are properly defined
#include "network_client.h"
#include "remote_debugger.hpp"
// Using the remote_debugger library which includes all necessary components

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
    void renderLocalUI();
    void renderRemoteFrameOnly();

    // Font texture management
    void updateFontTexture(const uint8_t* font_data, int width, int height);
    void cleanupFontTexture();

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

    // Create network processor if not exists
    if (!network_processor_) {
      network_processor_ =
          std::make_unique<spatial::debugger::NetPacketDispatcher>();

        // Set packet handler
      network_processor_->BindServicePacketHandler(spatial::debugger::NetServiceType::RemoteImgui,
          [this](spatial::debugger::NetServiceType service_type,
                 uint16_t service_cmd, const spatial::debugger::NetPacketBuffer& packet) {
            if (service_cmd == static_cast<uint32_t>(spatial::debugger::ImGuiCommand::FrameData)) {

                // Process ImGui draw data
                if (deserializer_.DeserializePacket(packet)) {
                    std::cout << "Received and deserialized ImGui frame data: " << packet.TotalSize() << " bytes" << std::endl;

                    // Store data for visualization
                    cached_packet_ = packet;
                } else {
                    std::cout << "Failed to deserialize ImGui frame data" << std::endl;
                }
            } else if (service_cmd == static_cast<uint32_t>(spatial::debugger::ImGuiCommand::FontTexture)) {
                // Process font texture data
                if (deserializer_.DeserializeFontTexture(packet)) {
                    std::cout << "Received and deserialized font texture data: " << packet.TotalSize() << " bytes" << std::endl;

                    // Update font texture in OpenGL
                    const auto* font_texture = deserializer_.GetFontTextureData();
                    if (font_texture) {
                        updateFontTexture(font_texture->pixel_data.data(),
                                        font_texture->header.width,
                                        font_texture->header.height);
                    }
                } else {
                    std::cout << "Failed to deserialize font texture data" << std::endl;
                }
            } else {
                std::cout << "Received other service data: "
                          << (int)service_type << " "
                          << ", cmd: " << service_cmd
                          << ", size: " << packet.TotalSize() << " bytes" << std::endl;
            }
        });
    }

    // Create network client if not exists
    if (!network_client_) {
        network_client_ = std::make_unique<NetworkClient>();

        // Set network callbacks
        network_client_->setConnectCallback([this](bool connected) {
            if (connected) {
                std::cout << "Successfully connected to server" << std::endl;

                ////// Send connect request
                ////auto connect_packet = ImDrawDataSerializer::createNetworkPacket(
                ////    ServiceType::IMGUI_DATA,
                ////    static_cast<uint32_t>(ImGuiCommand::REQUEST_CONNECT),
                ////    nullptr, 0
                ////);
                ////network_client_->send(connect_packet.data(), connect_packet.size());
            } else {
                std::cout << "Failed to connect to server" << std::endl;
            }
        });

        network_client_->setDisconnectCallback([this]() {
            std::cout << "Disconnected from server" << std::endl;
            //if (network_processor_) {
            //    network_processor_->Clear();
            //}
        });

        network_client_->setReceiveCallback([this](const uint8_t* data, size_t size) {
            // Process incoming data through network processor
            if (network_processor_) {
                network_processor_->AppendIncommingData(data, size);
                ////network_processor_->processIncomingData(data, size);
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

    // Cleanup network processor
    if (network_processor_) {
        //network_processor_->clear();
        network_processor_.reset();
    }

    if (window_) {
        // Clean up font texture before shutting down ImGui
        cleanupFontTexture();

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

    // Build font atlas to get texture ID
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

    // Set a placeholder texture ID initially
    io.Fonts->SetTexID(reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(0)));

    std::cout << "ImGui setup completed with OpenGL3 backend" << std::endl;
    std::cout << "Font atlas size: " << width << "x" << height << std::endl;
}

void SimpleOpenGLClient::renderFrame() {
    if (!initialized_ || !window_) {
        std::cerr << "Client not properly initialized!" << std::endl;
        return;
    }

    // Clear and setup viewport
    int display_w, display_h;
    glfwGetFramebufferSize(window_, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color_.x * clear_color_.w, clear_color_.y * clear_color_.w, clear_color_.z * clear_color_.w, clear_color_.w);
    glClear(GL_COLOR_BUFFER_BIT);

    // If we have remote frame data, render it directly
    if (deserializer_.HasValidFrame()) {
      renderRemoteFrameOnly();
    }
    else {
      // Only render local UI when no remote data
      renderLocalUI();
    }


    // Swap buffers
    glfwSwapBuffers(window_);
}

void SimpleOpenGLClient::renderLocalUI() {
    // Start ImGui frame for local UI
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Only show local UI (network settings) when no remote data
    ImGui::Text("Waiting for data from server...");
    ImGui::Text("Connect to a server to receive ImGui content");

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
            cached_packet_.Clear();
            ////received_data_.clear();
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
        ImGui::Text("Data received: %zu bytes", cached_packet_.TotalSize());

        ImGui::End();
    }

    // Render local UI
    ImGui::Render();
    if (ImGui::GetDrawData() != nullptr) {
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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

void SimpleOpenGLClient::renderRemoteFrameOnly() {
    if (!deserializer_.HasValidFrame()) {
        return;
    }

    const spatial::debugger::FrameData* frame_data = deserializer_.GetFrameData();
    if (!frame_data || frame_data->header.cmd_lists_count == 0) {
        return;
    }

    // Create a standalone ImDrawData structure from remote frame data
    ImDrawData remote_draw_data;
    remote_draw_data.Valid = true;
    remote_draw_data.CmdListsCount = frame_data->header.cmd_lists_count;
    remote_draw_data.CmdLists = nullptr;
    remote_draw_data.TotalIdxCount = 0;
    remote_draw_data.TotalVtxCount = 0;
    remote_draw_data.DisplayPos = ImVec2(frame_data->header.display_pos[0], frame_data->header.display_pos[1]);
    remote_draw_data.DisplaySize = ImVec2(frame_data->header.display_size[0], frame_data->header.display_size[1]);
    remote_draw_data.FramebufferScale = ImVec2(frame_data->header.framebuffer_scale[0], frame_data->header.framebuffer_scale[1]);

    // Update ImGui IO with server display settings
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = remote_draw_data.DisplaySize;
    io.DisplayFramebufferScale = remote_draw_data.FramebufferScale;

    // Allocate draw lists array
    remote_draw_data.CmdLists = new ImDrawList*[frame_data->header.cmd_lists_count];

    // Create draw lists from remote data
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

        // Determine vertex and index count for this list
        size_t next_vertex_offset = (i + 1 < frame_data->vertex_offsets.size()) ?
                                  frame_data->vertex_offsets[i + 1] : frame_data->vertex_buffers.size();
        size_t next_index_offset = (i + 1 < frame_data->index_offsets.size()) ?
                                  frame_data->index_buffers[i + 1] : frame_data->index_buffers.size();

        size_t list_vtx_count = next_vertex_offset - vertex_offset;
        size_t list_idx_count = next_index_offset - index_offset;

        if (list_vtx_count == 0 || list_idx_count == 0) {
            remote_draw_data.CmdLists[i] = nullptr;
            continue;
        }

        // Create new draw list
        ImDrawList* new_list = new ImDrawList(ImGui::GetDrawListSharedData());

        // Allocate and copy vertex buffer
        new_list->VtxBuffer.resize(list_vtx_count);
        for (size_t v = 0; v < list_vtx_count; v++) {
            if (vertex_offset + v < frame_data->vertex_buffers.size()) {
                new_list->VtxBuffer[v] = frame_data->vertex_buffers[vertex_offset + v];
            }
        }

        // Allocate and copy index buffer
        new_list->IdxBuffer.resize(list_idx_count);
        for (size_t idx = 0; idx < list_idx_count; idx++) {
            if (index_offset + idx < frame_data->index_buffers.size()) {
                new_list->IdxBuffer[idx] = frame_data->index_buffers[index_offset + idx];
            }
        }

        // Copy draw commands
        new_list->CmdBuffer.resize(cmd_count);
        for (uint32_t j = 0; j < cmd_count; j++) {
            if (command_offset + j < frame_data->draw_commands.size()) {
                const spatial::debugger::DrawCmd& src_cmd = frame_data->draw_commands[command_offset + j];
                ImDrawCmd& dst_cmd = new_list->CmdBuffer[j];

                dst_cmd.ElemCount = src_cmd.idx_count;
                dst_cmd.ClipRect.x = src_cmd.clip_rect[0] / 1000.0f;
                dst_cmd.ClipRect.y = src_cmd.clip_rect[1] / 1000.0f;
                dst_cmd.ClipRect.z = src_cmd.clip_rect[2] / 1000.0f;
                dst_cmd.ClipRect.w = src_cmd.clip_rect[3] / 1000.0f;
                // Map texture IDs: if it's the default font texture (ID 1), use our client font texture
                if (src_cmd.texture_id == 1) {
                    dst_cmd.TextureId = reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(font_texture_id_));
                    // Debug: Only log the first font texture mapping to avoid spam
                    static bool font_texture_logged = false;
                    if (!font_texture_logged) {
                        std::cout << "Mapped server font texture ID " << src_cmd.texture_id
                                  << " to client texture ID " << font_texture_id_ << std::endl;
                        font_texture_logged = true;
                    }
                } else {
                    dst_cmd.TextureId = (ImTextureID)(uintptr_t)src_cmd.texture_id;
                }
                dst_cmd.IdxOffset = 0;
                dst_cmd.VtxOffset = 0;

                // Handle UserCallback restoration
                if (src_cmd.user_callback != 0) {
                    // For remote ImGui, we need to provide appropriate callbacks
                    // Since we can't serialize function pointers, we'll provide standard callbacks
                    if (src_cmd.user_callback == 1) {
                        // This was a UserCallback - provide a default rendering callback
                        dst_cmd.UserCallback = ImDrawCallback_ResetRenderState;

                        // Allocate and copy user callback data
                        if (src_cmd.user_callback_data_size > 0 && !src_cmd.user_callback_data.empty()) {
                            // Allocate memory for callback data (will be freed by ImGui)
                            void* callback_data = malloc(src_cmd.user_callback_data_size);
                            if (callback_data) {
                                memcpy(callback_data, src_cmd.user_callback_data.data(), src_cmd.user_callback_data_size);
                                dst_cmd.UserCallbackData = callback_data;
                            } else {
                                dst_cmd.UserCallbackData = nullptr;
                            }
                        } else {
                            dst_cmd.UserCallbackData = nullptr;
                        }
                    } else {
                        dst_cmd.UserCallback = nullptr;
                        dst_cmd.UserCallbackData = nullptr;
                    }
                } else {
                    dst_cmd.UserCallback = nullptr;
                    dst_cmd.UserCallbackData = nullptr;
                }
            }
        }

        remote_draw_data.CmdLists[i] = new_list;
        remote_draw_data.TotalVtxCount += list_vtx_count;
        remote_draw_data.TotalIdxCount += list_idx_count;
    }

    // Debug: Print texture binding information before rendering
    std::cout << "Rendering remote frame data with " << remote_draw_data.CmdListsCount << " draw lists" << std::endl;
    if (font_texture_id_ != 0) {
        std::cout << "Font texture ID available: " << font_texture_id_ << std::endl;

        // Ensure font texture is bound properly
        glBindTexture(GL_TEXTURE_2D, font_texture_id_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        std::cout << "Warning: No font texture available!" << std::endl;
    }

    // Render the remote frame data directly
    ImGui_ImplOpenGL3_RenderDrawData(&remote_draw_data);

    // Clean up allocated draw lists and user callback data
    for (int i = 0; i < remote_draw_data.CmdListsCount; i++) {
        if (remote_draw_data.CmdLists[i]) {
            // Free UserCallbackData memory
            for (int cmd_idx = 0; cmd_idx < remote_draw_data.CmdLists[i]->CmdBuffer.Size; cmd_idx++) {
                ImDrawCmd& cmd = remote_draw_data.CmdLists[i]->CmdBuffer[cmd_idx];
                if (cmd.UserCallbackData) {
                    free(cmd.UserCallbackData);
                    cmd.UserCallbackData = nullptr;
                }
            }
            delete remote_draw_data.CmdLists[i];
        }
    }
    delete[] remote_draw_data.CmdLists;

    // Increment frame counter
    frame_count_++;
}

std::vector<uint8_t> SimpleOpenGLClient::ConvertAlphaToRgbaWhite(const uint8_t* alpha_data, int width, int height) {
    if (!alpha_data || width <= 0 || height <= 0) {
        return {};
    }

    size_t pixel_count = static_cast<size_t>(width) * height;

    std::vector<uint8_t> rgba_data(pixel_count * 4);

    for (size_t i = 0; i < pixel_count; ++i) {
        uint8_t alpha = alpha_data[i];
        rgba_data[i * 4 + 0] = 255;   // Red
        rgba_data[i * 4 + 1] = 255;   // Green
        rgba_data[i * 4 + 2] = 255;   // Blue
        rgba_data[i * 4 + 3] = alpha; // Alpha
    }
    return rgba_data;
}

void SimpleOpenGLClient::updateFontTexture(const uint8_t* font_data, int width, int height) {
    if (font_data == nullptr || width <= 0 || height <= 0) {
        std::cerr << "Invalid font texture data: " << (void*)font_data << ", " << width << "x" << height << std::endl;
        return;
    }

    auto rgba_data = ConvertAlphaToRgbaWhite(font_data, width, height);



    std::cout << "Updating font texture: " << width << "x" << height << std::endl;

    // Debug: Check first few pixels to verify data integrity
    std::cout << "Received first few pixel values: ";
    for (int i = 0; i < std::min(16, width * height); i++) {
        printf("%02X ", font_data[i]);
    }
    std::cout << std::endl;

    // Generate or update font texture
    if (font_texture_id_ == 0) {
        glGenTextures(1, &font_texture_id_);
        std::cout << "Generated new font texture ID: " << font_texture_id_ << std::endl;
    }

    glBindTexture(GL_TEXTURE_2D, font_texture_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    // Upload font data (Alpha8 format)
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    // Check for OpenGL errors before texture upload
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "OpenGL error before texture upload: " << err << std::endl;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba_data.data());

    // Check for OpenGL errors after texture upload
    err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "OpenGL error during texture upload: " << err << std::endl;
    } else {
        std::cout << "Texture uploaded successfully" << std::endl;
    }

    // Update ImGui's font atlas texture ID without rebuilding the atlas
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->SetTexID(reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(font_texture_id_)));

    // Verify OpenGL texture was created successfully
    GLint current_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &current_texture);
    std::cout << "Updated ImGui font atlas texture ID to: " << font_texture_id_
              << ", current bound texture: " << current_texture << std::endl;

    // Make sure our texture is bound
    if (current_texture != static_cast<GLint>(font_texture_id_)) {
        glBindTexture(GL_TEXTURE_2D, font_texture_id_);
        std::cout << "Re-bound font texture " << font_texture_id_ << std::endl;
    }

    // Verify texture was uploaded correctly by reading back a small sample
    std::vector<unsigned char> read_back_data(width * height);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_ALPHA, GL_UNSIGNED_BYTE, read_back_data.data());

    // Compare first few pixels
    bool data_matches = true;
    for (int i = 0; i < std::min(16, width * height); i++) {
        if (read_back_data[i] != font_data[i]) {
            data_matches = false;
            break;
        }
    }

    if (data_matches) {
        std::cout << "Texture data verification: PASSED" << std::endl;
    } else {
        std::cout << "Texture data verification: FAILED - data mismatch" << std::endl;
        std::cout << "Expected first few values: ";
        for (int i = 0; i < std::min(8, width * height); i++) {
            printf("%02X ", font_data[i]);
        }
        std::cout << std::endl;
        std::cout << "Read back first few values: ";
        for (int i = 0; i < std::min(8, width * height); i++) {
            printf("%02X ", read_back_data[i]);
        }
        std::cout << std::endl;
    }

    std::cout << "Font texture updated successfully" << std::endl;
}

void SimpleOpenGLClient::cleanupFontTexture() {
    if (font_texture_id_ != 0) {
        glDeleteTextures(1, &font_texture_id_);
        font_texture_id_ = 0;
        std::cout << "Cleaned up font texture" << std::endl;
    }
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