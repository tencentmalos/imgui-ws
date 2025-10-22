// Simple OpenGL client for remote ImGui
// Based on Dear ImGui example but simplified for compatibility

#include "imgui.h"
#include "deserializer.h"

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <thread>
#include <chrono>

// We'll use basic OpenGL without complex extensions for simplicity
#ifdef _WIN32
#include <windows.h>
#include <GL/gl.h>
#else
#include <GL/gl.h>
#endif

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
    // Basic window simulation using console output
    void setupImGui();
    void renderFrame();
    void renderImGui();

    // Members
    int window_width_ = 1280;
    int window_height_ = 720;
    std::string window_title_;
    bool initialized_ = false;
    ImGuiContext* imgui_context_ = nullptr;

    // ImGui state
    ImVec4 clear_color_ = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Data reception
    std::vector<uint8_t> received_data_;
    ImDrawDataDeserializer deserializer_;

    // Window flags
    bool show_demo_window_ = true;
    bool show_another_window_ = false;
    bool show_network_window_ = true;

    // Frame counter
    int frame_count_ = 0;
};

SimpleOpenGLClient::SimpleOpenGLClient() {
    imgui_context_ = nullptr;
}

SimpleOpenGLClient::~SimpleOpenGLClient() {
    cleanup();
}

bool SimpleOpenGLClient::initialize(int window_width, int window_height, const char* title) {
    window_width_ = window_width;
    window_height_ = window_height;
    window_title_ = title;

    // Setup ImGui
    setupImGui();

    initialized_ = true;
    std::cout << "Simple OpenGL client initialized successfully" << std::endl;
    return true;
}

bool SimpleOpenGLClient::connect(const std::string& server_ip, int port) {
    std::cout << "Connecting to " << server_ip << ":" << port << " (demo mode)" << std::endl;

    // Simulate receiving some data
    received_data_.clear();
    for (int i = 0; i < 64; ++i) {
        received_data_.push_back(0x40 + (i % 32));
    }

    return true;
}

void SimpleOpenGLClient::setupImGui() {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    imgui_context_ = ImGui::CreateContext();
    ImGui::SetCurrentContext(imgui_context_);
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Load Fonts
    io.Fonts->AddFontDefault();

    // Build font atlas manually
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);
    io.Fonts->SetTexID((ImTextureID)(intptr_t)1); // Fake texture ID
    io.Fonts->ClearTexData();

    std::cout << "ImGui setup completed" << std::endl;
}

void SimpleOpenGLClient::cleanup() {
    if (imgui_context_) {
        ImGui::DestroyContext(imgui_context_);
        imgui_context_ = nullptr;
    }

    std::cout << "Simple OpenGL client cleaned up" << std::endl;
}

void SimpleOpenGLClient::renderFrame() {
    if (!initialized_ || !imgui_context_) {
        std::cerr << "Client not properly initialized!" << std::endl;
        return;
    }

    // Set current context
    ImGui::SetCurrentContext(imgui_context_);

    // Start new frame
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)window_width_, (float)window_height_);
    io.DeltaTime = 1.0f / 60.0f; // Fixed 60 FPS for demo

    ImGui::NewFrame();

    // Render ImGui
    renderImGui();

    // End frame
    ImGui::Render();

    // Get draw data for potential processing
    ImDrawData* draw_data = ImGui::GetDrawData();

    if (draw_data && draw_data->Valid && frame_count_ % 60 == 0) {
        // Print info every 60 frames
        std::cout << "Frame " << frame_count_ << ": "
                  << draw_data->CmdListsCount << " command lists, "
                  << draw_data->TotalVtxCount << " vertices, "
                  << draw_data->TotalIdxCount << " indices" << std::endl;
    }

    frame_count_++;
}

void SimpleOpenGLClient::renderImGui() {
    // Main window
    {
        ImGui::Begin("Remote ImGui Client - OpenGL Rendering");

        ImGui::Text("This is a simple OpenGL-based ImGui client for testing");
        ImGui::Text("Window size: %dx%d", window_width_, window_height_);
        ImGui::Text("Frame: %d", frame_count_);

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

        ImGui::Separator();

        ImGui::Text("Received Data Display:");

        // Display received data in a simple text format
        if (!received_data_.empty()) {
            if (ImGui::BeginChild("Data View", ImVec2(0, 150), true)) {
                ImGui::Text("Hex view (first 64 bytes):");

                for (size_t i = 0; i < std::min((size_t)64, received_data_.size()); i += 16) {
                    // Offset
                    ImGui::Text("%04zx: ", i);
                    ImGui::SameLine();

                    // Hex bytes
                    for (int j = 0; j < 16 && (i + j) < received_data_.size(); ++j) {
                        ImGui::Text("%02X ", received_data_[i + j]);
                        ImGui::SameLine();
                    }

                    // Fill remaining space
                    for (int j = 0; j < 16 && (i + j) < received_data_.size(); ++j) {
                        // Already printed above
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
            for (int i = 0; i < 128; ++i) {
                received_data_.push_back(0x20 + ((i + pattern) % 96));
            }
            pattern = (pattern + 1) % 4;
            std::cout << "Simulated data reception: " << received_data_.size() << " bytes" << std::endl;
        }

        ImGui::Text("Note: This is a demo - OpenGL backend without visual rendering");

        ImGui::End();
    }

    // Demo window (simplified)
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
        ImGui::ColorEdit4("Color", (float*)&clear_color_);
        ImGui::End();
    }

    // Control window
    {
        ImGui::Begin("Controls");

        ImGui::Checkbox("Demo Window", &show_demo_window_);
        ImGui::Checkbox("Another Window", &show_another_window_);
        ImGui::Checkbox("Network Settings", &show_network_window_);

        ImGui::Separator();

        if (ImGui::Button("Exit")) {
            // In a real app, this would close the window
            std::cout << "Exit requested - ending simulation" << std::endl;
        }

        ImGui::Text("Press Ctrl+C to stop the simulation");

        ImGui::End();
    }
}

void SimpleOpenGLClient::run() {
    if (!initialized_) {
        std::cerr << "Client not initialized!" << std::endl;
        return;
    }

    std::cout << "Starting simple OpenGL ImGui client simulation..." << std::endl;
    std::cout << "This simulates ImGui rendering without actual OpenGL context" << std::endl;
    std::cout << "Press Ctrl+C to stop the simulation" << std::endl;

    // Simulate initial connection
    connect("127.0.0.1", 8080);

    // Main loop - run for demonstration
    for (int i = 0; i < 300; ++i) { // Run for 5 seconds at 60 FPS
        renderFrame();

        // Simulate frame rate timing
        std::this_thread::sleep_for(std::chrono::milliseconds(16));

        // Simulate data reception every 2 seconds
        if (i % 120 == 0) {
            // Simulate receiving new data
            static int data_pattern = 0;
            received_data_.clear();
            for (int j = 0; j < 64; ++j) {
                received_data_.push_back(0x30 + ((j + data_pattern) % 48));
            }
            data_pattern = (data_pattern + 1) % 4;
        }
    }

    std::cout << "Simulation completed after 5 seconds" << std::endl;
}

int main(int argc, char** argv) {
    SimpleOpenGLClient client;

    // Initialize client
    if (!client.initialize(1280, 720, "Remote ImGui Client - OpenGL Rendering")) {
        std::cerr << "Failed to initialize OpenGL client!" << std::endl;
        return -1;
    }

    // Run main loop
    client.run();

    return 0;
}