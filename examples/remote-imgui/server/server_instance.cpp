#include "server_instance.hpp"

// Network callbacks
void ServerInstance::onClientConnect(int client_id) {
    client_count++;
    std::cout << "Client connected: " << client_id << " (total: " << client_count.load() << ")"
        << std::endl;

    // Send font texture to newly connected client (if ready)
    sendFontTextureToClient();
}

void ServerInstance::onClientDisconnect(int client_id) {
    client_count--;
    std::cout << "Client disconnected: " << client_id << " (total: " << client_count.load()
        << ")" << std::endl;
}

void ServerInstance::onClientData(int client_id, const uint8_t* data, size_t size) {
    // Handle incoming data from clients (if needed)
    std::cout << "Received " << size << " bytes from client " << client_id << std::endl;
}

// Initialize server with networking
bool ServerInstance::initialize(int port) {
    serializer = std::make_unique<spatial::debugger::ImDrawDataSerializer>();

    // Create network server
    network_server = std::make_unique<NetworkServer>();

    // Set network callbacks
    network_server->setConnectCallback([this](int client_id) { onClientConnect(client_id); });

    network_server->setDisconnectCallback(
        [this](int client_id) { onClientDisconnect(client_id); });

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
void ServerInstance::prepareFontTexture() {
    std::lock_guard<std::mutex> lock(font_texture_mutex);

    if (font_texture_ready) {
        return;// Already prepared
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
        auto font_packet =
            serializer->getFontTexturePacket(font_texture_id, pixels, width, height, 0);

        // Cache the packet data for future retransmission
        auto full_packet_data = font_packet.GetFullPacketData();
        cached_font_texture_data.clear();
        cached_font_texture_data = full_packet_data;// Copy the vector

        font_texture_ready = true;
        std::cout << "Prepared font texture cache: " << width << "x" << height
            << ", data size: " << cached_font_texture_data.size() << " bytes"
            << std::endl;
    }
    else {
        std::cerr << "Failed to get font texture data from ImGui" << std::endl;
    }
}

// Send cached font texture to all connected clients
void ServerInstance::sendFontTextureToClient() {
    std::lock_guard<std::mutex> lock(font_texture_mutex);

    if (!font_texture_ready || !network_server) { return; }

    // Create packet from cached data
    spatial::debugger::NetPacketBuffer font_packet;
    font_packet.InitializeFromData(cached_font_texture_data.data(),
        cached_font_texture_data.size());

    // Broadcast to all connected clients
    network_server->broadcast(font_packet);

    std::cout << "Broadcasted cached font texture to " << client_count.load() << " clients"
        << std::endl;
}

void ServerInstance::broadcastDrawData() {
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

void ServerInstance::cleanup() {
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
