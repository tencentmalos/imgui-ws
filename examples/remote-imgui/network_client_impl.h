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

    // Connect to server
    bool connect(const std::string& host, int port);

    // Disconnect from server
    void disconnect();

    // Check if connected
    bool isConnected() const { return connected_; }

    // Send data to server
    bool send(const uint8_t* data, size_t size);

    // Set callbacks
    void setConnectCallback(ConnectCallback callback) { connect_callback_ = callback; }
    void setDisconnectCallback(DisconnectCallback callback) { disconnect_callback_ = callback; }
    void setReceiveCallback(DataReceiveCallback callback) { receive_callback_ = callback; }

    // Get connection info
    std::string getServerInfo() const;

    // Event loop integration
    void processEvents();  // Non-blocking event processing
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



    // Event callbacks
    static void readCallback(bufferevent* bev, void* ctx);
    static void eventCallback(bufferevent* bev, short events, void* ctx);

    // Internal methods
    void handleRead();
    void handleEvent(short events);
    void cleanup();
};