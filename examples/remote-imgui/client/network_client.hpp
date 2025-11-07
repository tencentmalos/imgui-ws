#pragma once

#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <vector>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include "net_packet_buffer.hpp"

// Forward declarations
struct event_base;
struct bufferevent;

class NetworkClient {
public:
    using ConnectCallback = std::function<void(bool connected)>;
    using DisconnectCallback = std::function<void()>;
    using DataReceiveCallback = std::function<void(const uint8_t* data, size_t size)>;

    NetworkClient();
    ~NetworkClient();

    bool Initialize();

    bool IsIntialized() const {
        return base_ != nullptr;
    }

    // Connect to server
    bool Connect(const std::string& host, int port);

    // Disconnect from server
    void Disconnect();

    // Check if connected
    bool IsConnected() const { return connected_; }

    bool SendPacket(const spatial::debugger::NetPacketBuffer& packet);

    // Set callbacks
    void SetConnectCallback(ConnectCallback callback) { connect_callback_ = callback; }
    void SetDisconnectCallback(DisconnectCallback callback) { disconnect_callback_ = callback; }
    void SetReceiveCallback(DataReceiveCallback callback) { receive_callback_ = callback; }

    // Get connection info
    std::string GetServerInfo() const;

    // Event loop integration
    void ProcessEvents();  // Non-blocking event processing
private:
    // Send data to server
    bool SendRawData(const uint8_t* data, size_t size);

    // Event callbacks
    static void ReadCallback(bufferevent* bev, void* ctx);
    static void EventCallback(bufferevent* bev, short events, void* ctx);

    // Internal methods
    void HandleRead();
    void HandleEvent(short events);

    void DestroyClientBev();

    void Cleanup();
private:
    // Libevent components
    event_base* base_;
    bufferevent* bev_;

    // Connection state
    std::atomic<bool> running_;
    std::atomic<bool> connected_;
    std::string server_host_;
    int server_port_;

    // Data buffer
    std::vector<uint8_t> receive_buffer_;
    mutable std::mutex buffer_mutex_;

    // Callbacks
    ConnectCallback connect_callback_;
    DisconnectCallback disconnect_callback_;
    DataReceiveCallback receive_callback_;




};