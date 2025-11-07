#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "imgui.h"
#include "network_server.hpp"
#include "remote_debugger.hpp"

// Global server instance
struct ServerInstance {
public:
    // Network callbacks
    void onClientConnect(int client_id);

    void onClientDisconnect(int client_id);

    void onClientData(int client_id, const uint8_t* data, size_t size);

    // Initialize server with networking
    bool initialize(int port);

    // Prepare and cache font texture data
    void prepareFontTexture();

    // Send cached font texture to all connected clients
    void sendFontTextureToClient();

    void broadcastDrawData();

    // Individual input event handling
    void applyMouseMoveEvent(const spatial::debugger::MouseMoveEvent& event);
    void applyMouseButtonEvent(const spatial::debugger::MouseButtonEvent& event);
    void applyMouseWheelEvent(const spatial::debugger::MouseWheelEvent& event);
    void applyKeyboardEvent(const spatial::debugger::KeyboardEvent& event);
    void applyCharEvent(const spatial::debugger::CharEvent& event);

    // Input event cache management
    void applyCachedInputEvents();  // Apply all cached input events
    void clearInputCache();         // Clear input event cache

    void cleanup();

public:
    std::unique_ptr<spatial::debugger::ImDrawDataSerializer> serializer;
    std::unique_ptr<NetworkServer> network_server;
    std::atomic<int> client_count{0};

    // Font texture management
    uint32_t font_texture_id{1};                  // Use a fixed ID for font texture
    std::vector<uint8_t> cached_font_texture_data;// Cached font texture data for retransmission
    std::mutex font_texture_mutex;                // Mutex for thread-safe font texture access
    bool font_texture_ready{false};               // Whether font texture data is ready

    // Individual input event caches
    std::vector<spatial::debugger::MouseMoveEvent> mouse_move_event_cache;
    std::vector<spatial::debugger::MouseButtonEvent> mouse_button_event_cache;
    std::vector<spatial::debugger::MouseWheelEvent> mouse_wheel_event_cache;
    std::vector<spatial::debugger::KeyboardEvent> keyboard_event_cache;
    std::vector<spatial::debugger::CharEvent> char_event_cache;
    std::mutex input_cache_mutex;                 // Mutex for thread-safe input cache access
};