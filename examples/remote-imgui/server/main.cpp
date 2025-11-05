#include "imgui.h"
#include "network_server.h"
#include "remote_debugger.hpp"


#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <cstdio>

// Global server instance
struct ServerInstance {
    std::unique_ptr<spatial::debugger::ImDrawDataSerializer> serializer;
    std::unique_ptr<NetworkServer> network_server;
    std::atomic<int> client_count{0};

    // Font texture management
    uint32_t font_texture_id{1};  // Use a fixed ID for font texture
    std::vector<uint8_t> cached_font_texture_data;  // Cached font texture data for retransmission
    std::mutex font_texture_mutex;  // Mutex for thread-safe font texture access
    bool font_texture_ready{false};  // Whether font texture data is ready

    // Network callbacks
    void onClientConnect(int client_id) {
        client_count++;
        std::cout << "Client connected: " << client_id << " (total: " << client_count.load() << ")" << std::endl;

        // Send font texture to newly connected client (if ready)
        sendFontTextureToClient();
    }

    void onClientDisconnect(int client_id) {
        client_count--;
        std::cout << "Client disconnected: " << client_id << " (total: " << client_count.load() << ")" << std::endl;
    }

    void onClientData(int client_id, const uint8_t* data, size_t size) {
        // Handle incoming data from clients (if needed)
        std::cout << "Received " << size << " bytes from client " << client_id << std::endl;
    }

    // Initialize server with networking
    bool initialize(int port) {
        serializer = std::make_unique<spatial::debugger::ImDrawDataSerializer>();

        // Create network server
        network_server = std::make_unique<NetworkServer>();

        // Set network callbacks
        network_server->setConnectCallback([this](int client_id) {
            onClientConnect(client_id);
        });

        network_server->setDisconnectCallback([this](int client_id) {
            onClientDisconnect(client_id);
        });

        network_server->setReceiveCallback([this](int client_id, const uint8_t* data, size_t size) {
            onClientData(client_id, data, size);
        });

        // Start network server
        if (!network_server->start(port)) {
            std::cerr << "Failed to start network server on port " << port << std::endl;
            return false;
        }

        std::cout << "Server initialized with networking on port " << port << std::endl;
        return true;
    }

    // Prepare and cache font texture data
    void prepareFontTexture() {
        std::lock_guard<std::mutex> lock(font_texture_mutex);

        if (font_texture_ready) {
            return; // Already prepared
        }

        if (!serializer) {
            std::cerr << "Serializer not ready for font texture preparation" << std::endl;
            return;
        }

        // Force rebuild font atlas to get fresh data
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();
        io.Fonts->AddFontDefault();
        io.Fonts->Build();

        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

        std::cout << "Font atlas rebuilt: " << width << "x" << height << std::endl;

        // Debug: Check first few pixels to verify data
        if (pixels && width > 0 && height > 0) {
            std::cout << "First pixel value: " << (int)pixels[0] << std::endl;
        }

        if (pixels && width > 0 && height > 0) {
            // Create font texture packet
            auto font_packet = serializer->getFontTexturePacket(font_texture_id, pixels, width, height, 0);

            // Cache the packet data for future retransmission
            auto full_packet_data = font_packet.GetFullPacketData();
            cached_font_texture_data.clear();
            cached_font_texture_data = full_packet_data;  // Copy the vector

            font_texture_ready = true;
            std::cout << "Prepared font texture cache: " << width << "x" << height
                      << ", data size: " << cached_font_texture_data.size() << " bytes" << std::endl;
        } else {
            std::cerr << "Failed to get font texture data from ImGui" << std::endl;
        }
    }

    // Send cached font texture to all connected clients
    void sendFontTextureToClient() {
        std::lock_guard<std::mutex> lock(font_texture_mutex);

        if (!font_texture_ready || !network_server) {
            return;
        }

        // Create packet from cached data
        spatial::debugger::NetPacketBuffer font_packet;
        font_packet.InitializeFromData(cached_font_texture_data.data(), cached_font_texture_data.size());

        // Broadcast to all connected clients
        network_server->broadcast(font_packet);

        std::cout << "Broadcasted cached font texture to " << client_count.load() << " clients" << std::endl;
    }

    void broadcastDrawData() {
        if (serializer && network_server && client_count.load() > 0) {
            // Ensure font texture is prepared and send to clients
            prepareFontTexture();

            // Send font texture to all clients (reliable transmission)
            sendFontTextureToClient();

            // Get packetized frame data
            auto packet = serializer->getPacketizedData();
            network_server->broadcast(packet);

            std::cout << "Broadcasted frame data (" << packet.TotalSize() << " bytes) to "
                        << client_count.load() << " clients" << std::endl;
        }
    }

    void cleanup() {
        if (network_server) {
            network_server->stop();
            network_server.reset();
        }

        std::lock_guard<std::mutex> lock(font_texture_mutex);
        cached_font_texture_data.clear();
        font_texture_ready = false;

        serializer.reset();
        client_count = 0;
        std::cout << "Server Cleanup completed" << std::endl;
    }
};

// Global server instance
static ServerInstance g_server;

int main(int argc, char** argv) {
    printf("Usage: %s [port]\n", argv[0]);

#ifdef WIN32
    // 1. Declare a WSADATA structure
    WSADATA wsaData;
    // 2. Request Winsock version 2.2 and initialize it
    // MAKEWORD(2, 2) creates the version number WORD.
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
      // WSAStartup failed.
      std::cerr << "WSAStartup failed with error code: " << result << std::endl;
      return 1; // Exit the application
    }
#endif

    int port = 8080;
    if (argc > 1) port = atoi(argv[1]);

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    io.DisplaySize = ImVec2(1280, 720);

    ImGui::StyleColorsDark();

    // Setup font texture
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

    // Initialize server
    if (!g_server.initialize(port)) {
        fprintf(stderr, "Failed to Initialize server on port %d\n", port);
        return -1;
    }

    printf("Remote ImGui Server started on port %d\n", port);
    printf("Press Ctrl+C to stop\n");

    // Server main loop
    bool running = true;
    static float f = 0.0f;
    static int counter = 0;
    static bool show_demo_window = false;

    while (running) {
        // Process network events
        g_server.network_server->processEvents();

        // Start new frame
        ImGui::NewFrame();

        // Create ImGui interface
        {
            ImGui::Begin("Hello, world!");
            ImGui::Text("This is a Remote ImGui Server!");
            ImGui::Text("Connected clients: %d", g_server.client_count.load());
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                       1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);

            static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
            ImGui::ColorEdit3("Clear color", (float*)&clear_color);

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
            static bool demo_open = true;
            ImGui::ShowDemoWindow(&demo_open);
        }

        // Render ImGui
        ImGui::Render();

        // Get draw data
        ImDrawData* draw_data = ImGui::GetDrawData();
        if (draw_data && draw_data->CmdListsCount > 0) {
            // Serialize draw data
            g_server.serializer->setDrawData(draw_data);

            // Broadcast to all clients
            g_server.broadcastDrawData();
        }

        // Simple delay
        std::this_thread::sleep_for(std::chrono::milliseconds(16));

        // Check exit condition (simplified version)
        // In real implementation, should handle Ctrl+C signal
        if (counter > 1000) { // Simple exit condition
            running = false;
        }
    }

    // Cleanup
    ImGui::DestroyContext();
    g_server.cleanup();

#ifdef WIN32
    // 4. When your application is done with all network operations, call WSACleanup()
    WSACleanup();
#endif


    printf("Server stopped\n");
    return 0;
}