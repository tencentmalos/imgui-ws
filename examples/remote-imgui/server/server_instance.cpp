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
    // Handle incoming data from clients
    std::cout << "Received " << size << " bytes from client " << client_id << std::endl;

    // Try to deserialize as input events
    spatial::debugger::NetPacketBuffer packet;
    packet.InitializeFromData(data, size);

    // Get packet header to determine command type
    auto header = packet.GetHeader();
    if (header.service_type == (int)spatial::debugger::NetServiceType::RemoteImgui) {
        spatial::debugger::ImDrawDataDeserializer deserializer;

        std::lock_guard<std::mutex> lock(input_cache_mutex);

        switch (header.service_cmd) {
            case static_cast<uint16_t>(spatial::debugger::ImGuiCommand::MouseMoveEvent):
            {
                spatial::debugger::MouseMoveEvent event;
                if (deserializer.DeserializeMouseMoveEvent(packet, event)) {
                    mouse_move_event_cache.push_back(event);
                    std::cout << "Cached mouse move event from client " << client_id << std::endl;
                }
                break;
            }
            case static_cast<uint16_t>(spatial::debugger::ImGuiCommand::MouseButtonEvent):
            {
                spatial::debugger::MouseButtonEvent event;
                if (deserializer.DeserializeMouseButtonEvent(packet, event)) {
                    mouse_button_event_cache.push_back(event);
                    std::cout << "Cached mouse button event from client " << client_id << std::endl;
                }
                break;
            }
            case static_cast<uint16_t>(spatial::debugger::ImGuiCommand::MouseWheelEvent):
            {
                spatial::debugger::MouseWheelEvent event;
                if (deserializer.DeserializeMouseWheelEvent(packet, event)) {
                    mouse_wheel_event_cache.push_back(event);
                    std::cout << "Cached mouse wheel event from client " << client_id << std::endl;
                }
                break;
            }
            case static_cast<uint16_t>(spatial::debugger::ImGuiCommand::KeyboardEvent):
            {
                spatial::debugger::KeyboardEvent event;
                if (deserializer.DeserializeKeyboardEvent(packet, event)) {
                    keyboard_event_cache.push_back(event);
                    std::cout << "Cached keyboard event from client " << client_id << std::endl;
                }
                break;
            }
            case static_cast<uint16_t>(spatial::debugger::ImGuiCommand::CharEvent):
            {
                spatial::debugger::CharEvent event;
                if (deserializer.DeserializeCharEvent(packet, event)) {
                    char_event_cache.push_back(event);
                    std::cout << "Cached char event from client " << client_id << std::endl;
                }
                break;
            }
            default:
                std::cout << "Unknown command type: " << header.service_cmd << std::endl;
                break;
        }
    }
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

    std::lock_guard<std::mutex> font_lock(font_texture_mutex);
    cached_font_texture_data.clear();
    font_texture_ready = false;

    std::lock_guard<std::mutex> input_lock(input_cache_mutex);
    mouse_move_event_cache.clear();
    mouse_button_event_cache.clear();
    mouse_wheel_event_cache.clear();
    keyboard_event_cache.clear();
    char_event_cache.clear();

    serializer.reset();
    client_count = 0;
    std::cout << "Server Cleanup completed" << std::endl;
}


// Individual input event application methods
void ServerInstance::applyMouseMoveEvent(const spatial::debugger::MouseMoveEvent& event) {
    if (!ImGui::GetCurrentContext()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(static_cast<float>(event.x), static_cast<float>(event.y));
}

void ServerInstance::applyMouseButtonEvent(const spatial::debugger::MouseButtonEvent& event) {
    if (!ImGui::GetCurrentContext()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    bool pressed = (event.action == spatial::debugger::MouseButtonAction::Press);
    io.AddMouseButtonEvent(static_cast<int>(event.button), pressed);

    // Apply modifier states
    io.KeyShift = event.shift_pressed;
    io.KeyCtrl = event.ctrl_pressed;
    io.KeyAlt = event.alt_pressed;
    io.KeySuper = event.super_pressed;
}

void ServerInstance::applyMouseWheelEvent(const spatial::debugger::MouseWheelEvent& event) {
    if (!ImGui::GetCurrentContext()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseWheelEvent(static_cast<float>(event.x_offset), static_cast<float>(event.y_offset));
    io.AddMousePosEvent(static_cast<float>(event.mouse_x), static_cast<float>(event.mouse_y));
}

void ServerInstance::applyKeyboardEvent(const spatial::debugger::KeyboardEvent& event) {
    if (!ImGui::GetCurrentContext()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    bool pressed = (event.action == spatial::debugger::KeyAction::Press);
    io.AddKeyEvent(static_cast<ImGuiKey>(event.key_code), pressed);

    // Apply modifier states
    io.KeyShift = event.shift_pressed;
    io.KeyCtrl = event.ctrl_pressed;
    io.KeyAlt = event.alt_pressed;
    io.KeySuper = event.super_pressed;
}

void ServerInstance::applyCharEvent(const spatial::debugger::CharEvent& event) {
    if (!ImGui::GetCurrentContext()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharacter(static_cast<ImWchar>(event.char_code));
}

void ServerInstance::applyCachedInputEvents() {
    std::lock_guard<std::mutex> lock(input_cache_mutex);

    size_t total_applied = 0;

    // Apply mouse move events (only the latest one to avoid spam)
    if (!mouse_move_event_cache.empty()) {
        applyMouseMoveEvent(mouse_move_event_cache.back());
        total_applied += mouse_move_event_cache.size();
    }

    // Apply mouse button events
    for (const auto& event : mouse_button_event_cache) {
        applyMouseButtonEvent(event);
        total_applied++;
    }

    // Apply mouse wheel events
    for (const auto& event : mouse_wheel_event_cache) {
        applyMouseWheelEvent(event);
        total_applied++;
    }

    // Apply keyboard events
    for (const auto& event : keyboard_event_cache) {
        applyKeyboardEvent(event);
        total_applied++;
    }

    // Apply char events
    for (const auto& event : char_event_cache) {
        applyCharEvent(event);
        total_applied++;
    }

    if (total_applied > 0) {
        std::cout << "Applied " << total_applied << " cached input events to ImGui" << std::endl;
    }
}

void ServerInstance::clearInputCache() {
    std::lock_guard<std::mutex> lock(input_cache_mutex);

    size_t total_cleared = mouse_move_event_cache.size() +
                         mouse_button_event_cache.size() +
                         mouse_wheel_event_cache.size() +
                         keyboard_event_cache.size() +
                         char_event_cache.size();

    mouse_move_event_cache.clear();
    mouse_button_event_cache.clear();
    mouse_wheel_event_cache.clear();
    keyboard_event_cache.clear();
    char_event_cache.clear();

    if (total_cleared > 0) {
        std::cout << "Cleared " << total_cleared << " cached input events" << std::endl;
    }
}
