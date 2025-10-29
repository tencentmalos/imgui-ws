#include "network_client_impl.h"
#include <iostream>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

NetworkClient::NetworkClient()
    : base_(nullptr)
    , bev_(nullptr)
    , running_(false)
    , connected_(false)
    , server_port_(0) {
}

NetworkClient::~NetworkClient() {
    disconnect();
}

bool NetworkClient::connect(const std::string& host, int port) {
    if (running_ && connected_) {
        std::cerr << "Client already connected" << std::endl;
        return false;
    }

    server_host_ = host;
    server_port_ = port;

    // Create event base
    base_ = event_base_new();
    if (!base_) {
        std::cerr << "Failed to create event base" << std::endl;
        return false;
    }

    // Create bufferevent
    bev_ = bufferevent_socket_new(base_, -1, BEV_OPT_CLOSE_ON_FREE);
    if (!bev_) {
        std::cerr << "Failed to create bufferevent" << std::endl;
        event_base_free(base_);
        base_ = nullptr;
        return false;
    }

    // Set callbacks
    bufferevent_setcb(bev_, readCallback, nullptr, eventCallback, this);
    bufferevent_enable(bev_, EV_READ | EV_WRITE);

    // Start connection
    sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);

    // Convert hostname to IP address
    if (inet_pton(AF_INET, host.c_str(), &sin.sin_addr) <= 0) {
        std::cerr << "Invalid IP address: " << host << std::endl;
        bufferevent_free(bev_);
        bev_ = nullptr;
        event_base_free(base_);
        base_ = nullptr;
        return false;
    }

    if (bufferevent_socket_connect(bev_, (sockaddr*)&sin, sizeof(sin)) < 0) {
        std::cerr << "Failed to initiate connection" << std::endl;
        bufferevent_free(bev_);
        bev_ = nullptr;
        event_base_free(base_);
        base_ = nullptr;
        return false;
    }

    running_ = true;

    // Don't start separate thread - we'll process events in main loop
    std::cout << "Connecting to " << host << ":" << port << std::endl;
    return true;
}

void NetworkClient::disconnect() {
    if (!running_) {
        return;
    }

    running_ = false;
    connected_ = false;

    std::cout << "Disconnecting from server" << std::endl;

    // Call disconnect callback
    if (disconnect_callback_) {
        disconnect_callback_();
    }

    // Cleanup
    cleanup();
}

bool NetworkClient::send(const uint8_t* data, size_t size) {
    if (!connected_ || !bev_ || !data || size == 0) {
        return false;
    }

    int result = bufferevent_write(bev_, data, size);
    return result == 0;
}

std::string NetworkClient::getServerInfo() const {
    if (connected_) {
        return server_host_ + ":" + std::to_string(server_port_);
    }
    return "Not connected";
}

void NetworkClient::processEvents() {
    if (base_ && running_) {
        // Non-blocking event processing
        event_base_loop(base_, EVLOOP_NONBLOCK);
    }
}

void NetworkClient::readCallback(bufferevent* bev, void* ctx) {
    NetworkClient* client = static_cast<NetworkClient*>(ctx);
    if (client) {
        client->handleRead();
    }
}

void NetworkClient::eventCallback(bufferevent* bev, short events, void* ctx) {
    NetworkClient* client = static_cast<NetworkClient*>(ctx);
    if (client) {
        client->handleEvent(events);
    }
}

void NetworkClient::handleRead() {
    if (!bev_) {
        return;
    }

    evbuffer* input = bufferevent_get_input(bev_);
    size_t len = evbuffer_get_length(input);

    if (len > 0) {
        std::vector<uint8_t> data(len);
        evbuffer_remove(input, data.data(), len);

        std::lock_guard<std::mutex> lock(buffer_mutex_);
        receive_buffer_.insert(receive_buffer_.end(), data.begin(), data.end());

        // Call receive callback
        if (receive_callback_) {
            receive_callback_(data.data(), len);
        }
    }
}

void NetworkClient::handleEvent(short events) {
    if (events & BEV_EVENT_CONNECTED) {
        connected_ = true;
        std::cout << "Connected to server" << std::endl;

        // Call connect callback
        if (connect_callback_) {
            connect_callback_(true);
        }
    }
    else if (events & (BEV_EVENT_ERROR | BEV_EVENT_EOF)) {
        if (events & BEV_EVENT_ERROR) {
            std::cerr << "Connection error: "
                      << evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()) << std::endl;
        }
        else {
            std::cout << "Connection closed by server" << std::endl;
        }

        connected_ = false;

        // Call connect callback with false
        if (connect_callback_) {
            connect_callback_(false);
        }

        // Call disconnect callback
        if (disconnect_callback_) {
            disconnect_callback_();
        }

        // Cleanup
        cleanup();
    }
}

void NetworkClient::cleanup() {
    if (bev_) {
        bufferevent_free(bev_);
        bev_ = nullptr;
    }

    if (base_) {
        event_base_loopbreak(base_);
        event_base_free(base_);
        base_ = nullptr;
    }

    // Clear buffer
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    receive_buffer_.clear();
}