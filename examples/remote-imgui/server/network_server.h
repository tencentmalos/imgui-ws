#pragma once

#include <memory>
#include <thread>
#include <atomic>
#include <vector>
#include <functional>
#include <mutex>
#include <string>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include "../remote_debugger/net_packet_buffer.hpp"

// Forward declarations
struct evconnlistener;
struct event_base;
struct bufferevent;

class NetworkServer {
public:
    using ClientConnectCallback = std::function<void(int client_id)>;
    using ClientDisconnectCallback = std::function<void(int client_id)>;
    using DataReceiveCallback = std::function<void(int client_id, const uint8_t* data, size_t size)>;

    NetworkServer();
    ~NetworkServer();

    // Start server on specified port
    bool start(int port);

    // Stop server
    void stop();

    // Check if server is running
    bool isRunning() const { return running_; }

    // Get number of connected clients
    int getClientCount() const { return client_count_.load(); }

    // Send data to specific client
    bool sendToClient(int client_id, const uint8_t* data, size_t size);

    // Broadcast data to all connected clients
    bool broadcast(const uint8_t* data, size_t size);
    bool broadcast(const spatial::debugger::NetPacketBuffer& packet);

    // Set callbacks
    void setConnectCallback(ClientConnectCallback callback) { connect_callback_ = callback; }
    void setDisconnectCallback(ClientDisconnectCallback callback) { disconnect_callback_ = callback; }
    void setReceiveCallback(DataReceiveCallback callback) { receive_callback_ = callback; }

    // Event loop integration
    void processEvents();  // Non-blocking event processing

private:
    struct ClientConnection {
        int client_id;
        bufferevent* bev;
        std::string address;
        NetworkServer* server;
        ClientConnection(int id, bufferevent* b, const std::string& addr)
            : client_id(id), bev(b), address(addr), server(nullptr) {}
    };

    // Libevent components
    event_base* base_;
    evconnlistener* listener_;

    // Server state
    std::atomic<bool> running_;
    std::atomic<int> client_count_;
    int next_client_id_;

    // Client management
    std::vector<std::unique_ptr<ClientConnection>> clients_;
    mutable std::mutex clients_mutex_;

    // Callbacks
    ClientConnectCallback connect_callback_;
    ClientDisconnectCallback disconnect_callback_;
    DataReceiveCallback receive_callback_;


    // Event callbacks
    static void acceptCallback(evconnlistener* listener, evutil_socket_t fd,
                              sockaddr* addr, int socklen, void* ctx);
    static void readCallback(bufferevent* bev, void* ctx);
    static void eventCallback(bufferevent* bev, short events, void* ctx);

    // Internal methods
    void handleNewConnection(evutil_socket_t fd, sockaddr* addr, int socklen);
    void handleClientRead(ClientConnection* client);
    void handleClientEvent(ClientConnection* client, short events);
    void removeClient(ClientConnection* client);
    ClientConnection* findClient(int client_id);
    ClientConnection* findClientByBufferevent(bufferevent* bev);
};