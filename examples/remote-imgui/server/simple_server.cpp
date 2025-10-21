#include "imgui.h"
#include "serializer.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>

// Simple server instance without libevent dependencies
struct SimpleServerInstance {
    std::unique_ptr<ImDrawDataSerializer> serializer;
    std::atomic<int> client_count{0};

    void initialize() {
        serializer = std::make_unique<ImDrawDataSerializer>();
        std::cout << "Simple Server initialized" << std::endl;
    }

    void broadcastDrawData() {
        if (serializer) {
            size_t data_size = serializer->getDataSize();
            std::cout << "Broadcasting draw data: " << data_size << " bytes to "
                      << client_count.load() << " clients" << std::endl;

            // In real implementation, this would send serialized data to all clients
            const std::vector<uint8_t>& serialized_data = serializer->getSerializedData();
            if (!serialized_data.empty()) {
                std::cout << "Data ready for network transmission" << std::endl;
            }
        }
    }

    void cleanup() {
        serializer.reset();
        client_count = 0;
        std::cout << "Simple Server cleanup completed" << std::endl;
    }
};

// Global server instance
static SimpleServerInstance g_simple_server;

int main(int argc, char** argv) {
    printf("Usage: %s [port]\n", argv[0]);

    int port = 8080;
    if (argc > 1) port = atoi(argv[1]);

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    // Setup font texture
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

    // Initialize simple server
    g_simple_server.initialize();

    printf("Remote ImGui Simple Server started on port %d\n", port);
    printf("Press Ctrl+C to stop\n");
    printf("This version demonstrates ImGui serialization without networking\n");

    // Server main loop
    bool running = true;
    static float f = 0.0f;
    static int counter = 0;
    static bool show_demo_window = false;

    while (running) {
        // Start new frame
        ImGui::NewFrame();

        // Create ImGui interface
        {
            ImGui::Begin("Hello, world!");
            ImGui::Text("This is a Remote ImGui Server!");
            ImGui::Text("Connected clients: %d", g_simple_server.client_count.load());
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                       1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);

            static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
            ImGui::ColorEdit3("clear color", (float*)&clear_color);

            if (ImGui::Button("Button")) {
                counter++;
            }
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            if (ImGui::Button("Toggle Demo Window")) {
                show_demo_window = !show_demo_window;
            }

            ImGui::End();
        }

        // Demo window
        if (show_demo_window) {
            ImGui::ShowDemoWindow(&show_demo_window);
        }

        // Render ImGui
        ImGui::Render();

        // Get draw data and serialize
        ImDrawData* draw_data = ImGui::GetDrawData();
        if (draw_data && draw_data->CmdListsCount > 0) {
            // Serialize draw data
            g_simple_server.serializer->setDrawData(draw_data);

            // Broadcast to all clients
            g_simple_server.broadcastDrawData();
        }

        // Simple delay to simulate frame rate
        std::this_thread::sleep_for(std::chrono::milliseconds(16));

        // Simple exit condition
        if (counter > 1000) {
            running = false;
        }
    }

    // Cleanup
    ImGui::DestroyContext();
    g_simple_server.cleanup();

    printf("Simple Server stopped\n");
    return 0;
}