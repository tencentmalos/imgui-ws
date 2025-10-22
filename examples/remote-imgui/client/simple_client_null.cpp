// Simple null-backend client for remote ImGui
// Based on imgui-ws basic-null example - headless mode for testing

#include "imgui.h"
#include "deserializer.h"

#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cmath>

class SimpleNullClient {
public:
    SimpleNullClient();
    ~SimpleNullClient();

    bool initialize();
    void run();
    void cleanup();

    // Connect to server (without real networking for now)
    bool connect(const std::string& server_ip, int port);

private:
    // ImGui setup
    void setupImGui();

    // Main loop processing
    void processFrame();

    // Simulate data reception
    void simulateDataReception();

    // ImGui rendering
    void renderImGui();

    // Members
    bool initialized_ = false;
    ImGuiContext* imgui_context_ = nullptr;

    // Timing
    using clock = std::chrono::high_resolution_clock;
    using time_point = std::chrono::time_point<clock>;
    time_point last_frame_time_;

    // Display size (simulated)
    ImVec2 display_size_;

    // Data reception
    std::vector<uint8_t> received_data_;
    ImDrawDataDeserializer deserializer_;

    // Frame counter
    int frame_count_ = 0;
};

SimpleNullClient::SimpleNullClient() {
    imgui_context_ = nullptr;
    display_size_ = ImVec2(1280, 720);
    last_frame_time_ = clock::now();
}

SimpleNullClient::~SimpleNullClient() {
    cleanup();
}

bool SimpleNullClient::initialize() {
    // Initialize ImGui
    setupImGui();

    initialized_ = true;
    std::cout << "Null client initialized successfully (headless mode)" << std::endl;
    return true;
}

bool SimpleNullClient::connect(const std::string& server_ip, int port) {
    std::cout << "Connecting to " << server_ip << ":" << port << " (demo mode)" << std::endl;

    // Simulate some initial data reception
    simulateDataReception();

    return true;
}

void SimpleNullClient::setupImGui() {
    // Create ImGui context
    IMGUI_CHECKVERSION();
    imgui_context_ = ImGui::CreateContext();
    ImGui::SetCurrentContext(imgui_context_);

    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    // Configuration
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.DisplaySize = display_size_;

    // Style
    ImGui::StyleColorsDark();

    // Load fonts
    io.Fonts->AddFontDefault();

    // Build font atlas manually for null backend
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);
    io.Fonts->SetTexID((ImTextureID)(intptr_t)1); // Fake texture ID
    io.Fonts->ClearTexData(); // Clear CPU data

    std::cout << "ImGui setup completed (null backend)" << std::endl;
}

void SimpleNullClient::cleanup() {
    if (imgui_context_) {
        ImGui::DestroyContext(imgui_context_);
        imgui_context_ = nullptr;
    }

    std::cout << "Null client cleaned up" << std::endl;
}

void SimpleNullClient::processFrame() {
    if (!initialized_ || !imgui_context_) {
        std::cerr << "Client not properly initialized!" << std::endl;
        return;
    }

    // Set current context
    ImGui::SetCurrentContext(imgui_context_);

    // Calculate delta time
    auto now = clock::now();
    float delta_time = std::chrono::duration<float>(now - last_frame_time_).count();
    last_frame_time_ = now;

    // Update IO
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = delta_time > 0.0f ? delta_time : 1.0f / 60.0f;
    io.DisplaySize = display_size_;

    // Start new frame
    ImGui::NewFrame();

    // Render ImGui UI
    renderImGui();

    // End frame
    ImGui::Render();

    // Get draw data
    ImDrawData* draw_data = ImGui::GetDrawData();

    if (draw_data && draw_data->Valid) {
        // In null mode, we just print some info about the draw data
        if (frame_count_ % 60 == 0) { // Print every 60 frames
            std::cout << "Frame " << frame_count_ << ": "
                      << draw_data->CmdListsCount << " command lists, "
                      << draw_data->TotalVtxCount << " vertices, "
                      << draw_data->TotalIdxCount << " indices" << std::endl;
        }
    }

    frame_count_++;
}

void SimpleNullClient::simulateDataReception() {
    // Simulate receiving some binary data
    received_data_.clear();

    // Create a fake data packet with some recognizable pattern
    for (int i = 0; i < 32; ++i) {
        received_data_.push_back(0x40 + (i % 16)); // Some pattern
    }

    std::cout << "Simulated data reception: " << received_data_.size() << " bytes" << std::endl;
}

void SimpleNullClient::renderImGui() {
    // Main window
    {
        ImGui::Begin("Remote ImGui Client (Null Backend)");

        ImGui::Text("This is a headless ImGui client for testing");
        ImGui::Text("Display size: %.0fx%.0f", display_size_.x, display_size_.y);
        ImGui::Text("Frame: %d", frame_count_);

        ImGui::Separator();

        ImGui::Text("Data reception:");
        ImGui::SameLine();
        ImGui::Text("Received: %zu bytes", received_data_.size());

        ImGui::Separator();

        ImGui::Text("Controls:");
        ImGui::Text("- This is a null backend - no visual output");
        ImGui::Text("- Data processing happens in the background");
        ImGui::Text("- Press Ctrl+C to exit");

        ImGui::Separator();

        ImGui::Text("Received Data Display:");

        // Display received data in a simple text format
        if (!received_data_.empty()) {
            ImGui::BeginChild("Data View", ImVec2(0, ImGui::GetFrameHeight() - 80), true);

            ImGui::Text("First 32 bytes:");
            size_t display_size = std::min((size_t)32, received_data_.size());
            for (size_t i = 0; i < display_size; ++i) {
                ImGui::SameLine();
                ImGui::Text("0x%02X", received_data_[i] & 0xFF);
                if ((i + 1) % 16 == 0) ImGui::NewLine();
            }

            ImGui::Text("Total: %zu bytes", received_data_.size());

            ImGui::EndChild();
        } else {
            ImGui::Text("No data received yet");
        }

        ImGui::End();
    }

    // Control panel
    {
        ImGui::Begin("Network Settings");

        static char server_ip[256] = "127.0.0.1";
        static int server_port = 8080;

        ImGui::InputText("Server IP", server_ip, sizeof(server_ip));
        ImGui::SameLine();
        ImGui::InputInt("Server Port", &server_port);

        if (ImGui::Button("Connect")) {
            connect(server_ip, server_port);
        }

        if (ImGui::Button("Simulate Data")) {
            simulateDataReception();
        }

        ImGui::Text("Note: This is a null backend demo");
        ImGui::Text("No real networking or rendering implemented yet");

        ImGui::End();
    }

    // Demo windows (simplified)
    {
        ImGui::Begin("Demo Windows");

        static bool show_demo = false;
        static bool show_another = false;

        ImGui::Checkbox("Demo Window", &show_demo);
        ImGui::Checkbox("Another Window", &show_another);

        if (show_demo) {
            ImGui::Begin("Dear ImGui Demo", &show_demo);
            ImGui::ShowUserGuide();
            ImGui::Text("Demo window would show here");
            ImGui::BulletText("This is the null backend");
            ImGui::BulletText("No actual rendering occurs");
            ImGui::End();
        }

        if (show_another) {
            ImGui::Begin("Another Window", &show_another);
            ImGui::Text("This is another demo window");
            ImGui::Text("Frame count: %d", frame_count_);
            ImGui::End();
        }

        ImGui::End();
    }

    // Performance info
    {
        ImGui::Begin("Performance");

        ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                    1000.0f / io.Framerate, io.Framerate);
        ImGui::Text("Vertices: %d", io.MetricsRenderVertices);
        ImGui::Text("Indices: %d", io.MetricsRenderIndices);
        ImGui::Text("Windows: %d", io.MetricsRenderWindows);

        ImGui::End();
    }
}

void SimpleNullClient::run() {
    if (!initialized_) {
        std::cerr << "Client not initialized!" << std::endl;
        return;
    }

    std::cout << "Starting null ImGui client main loop..." << std::endl;
    std::cout << "This will run indefinitely. Press Ctrl+C to exit." << std::endl;

    // Simulate initial connection
    connect("127.0.0.1", 8080);

    while (true) {
        processFrame();

        // Simple frame rate limiting - ~60 FPS
        std::this_thread::sleep_for(std::chrono::milliseconds(16));

        // Simulate data reception every 5 seconds
        if (frame_count_ % 300 == 0) {
            simulateDataReception();
        }
    }

    std::cout << "Null client main loop ended" << std::endl;
}

int main(int argc, char** argv) {
    SimpleNullClient client;

    // Initialize client
    if (!client.initialize()) {
        std::cerr << "Failed to initialize client!" << std::endl;
        return -1;
    }

    // Run main loop
    client.run();

    return 0;
}