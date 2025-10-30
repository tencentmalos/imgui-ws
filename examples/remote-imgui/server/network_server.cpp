#include "network_server.h"

#include <cstring>
#include <iostream>
#include <mutex>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

NetworkServer::NetworkServer()
    : base_(nullptr),
      listener_(nullptr),
      running_(false),
      client_count_(0),
      next_client_id_(1) {}

NetworkServer::~NetworkServer() { stop(); }

bool NetworkServer::start(int port) {
  if (running_) {
    std::cerr << "Server is already running" << std::endl;
    return false;
  }

  // Create event base
  base_ = event_base_new();
  if (!base_) {
    std::cerr << "Failed to create event base" << std::endl;
    return false;
  }

  // Create address
  sockaddr_in sin;
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  sin.sin_port = htons(port);

  // Create listener
  listener_ = evconnlistener_new_bind(base_, acceptCallback, this,
                                      LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
                                      -1, (sockaddr*)&sin, sizeof(sin));
  if (!listener_) {
    std::cerr << "Failed to create listener" << std::endl;
    event_base_free(base_);
    base_ = nullptr;
    return false;
  }

  running_ = true;

  // Events will be processed in main loop
  std::cout << "Network server started on port " << port << std::endl;
  return true;
}

void NetworkServer::stop() {
  if (!running_) {
    return;
  }

  running_ = false;

  // Stop listener
  if (listener_) {
    evconnlistener_free(listener_);
    listener_ = nullptr;
  }

  // Stop event loop
  if (base_) {
    event_base_loopbreak(base_);
  }

  // Clean up event base
  if (base_) {
    event_base_free(base_);
    base_ = nullptr;
  }

  // Clean up clients
  std::lock_guard<std::mutex> lock(clients_mutex_);
  for (auto& client : clients_) {
    if (client->bev) {
      bufferevent_free(client->bev);
    }
  }
  clients_.clear();
  client_count_ = 0;

  std::cout << "Network server stopped" << std::endl;
}

bool NetworkServer::sendToClient(int client_id, const uint8_t* data,
                                 size_t size) {
  if (!running_ || !data || size == 0) {
    return false;
  }

  std::lock_guard<std::mutex> lock(clients_mutex_);
  ClientConnection* client = findClient(client_id);
  if (!client || !client->bev) {
    return false;
  }

  int result = bufferevent_write(client->bev, data, size);
  return result == 0;
}

bool NetworkServer::broadcast(const uint8_t* data, size_t size) {
  if (!running_ || !data || size == 0) {
    return false;
  }

  std::lock_guard<std::mutex> lock(clients_mutex_);
  bool success = true;

  for (auto& client : clients_) {
    if (client->bev) {
      int result = bufferevent_write(client->bev, data, size);
      if (result != 0) {
        success = false;
      }
    }
  }

  return success;
}

bool NetworkServer::broadcast(
    const spatial::debugger::NetPacketBuffer& packet) {
  if (!running_) [[unlikey]] {
    return false;
  }

  std::lock_guard<std::mutex> lock(clients_mutex_);
  bool success = true;

  for (auto& client : clients_) {
    auto& header = packet.GetHeader();
    auto& content = packet.GetContents();

    if (client->bev) {
      int result = bufferevent_write(client->bev, &header, sizeof(header));
      if (result != 0) {
        success = false;
        continue;
      }

      result = bufferevent_write(client->bev, content.data(), content.size());
      if (result != 0) {
        success = false;
        continue;
      }
    }
  }

  return success;
}

void NetworkServer::processEvents() {
  if (base_ && running_) {
    // Non-blocking event processing
    event_base_loop(base_, EVLOOP_NONBLOCK);
  }
}

void NetworkServer::acceptCallback(evconnlistener* listener, evutil_socket_t fd,
                                   sockaddr* addr, int socklen, void* ctx) {
  NetworkServer* server = static_cast<NetworkServer*>(ctx);
  server->handleNewConnection(fd, addr, socklen);
}

void NetworkServer::readCallback(bufferevent* bev, void* ctx) {
  ClientConnection* client = static_cast<ClientConnection*>(ctx);
  if (client && client->server) {
    client->server->handleClientRead(client);
  }
}

void NetworkServer::eventCallback(bufferevent* bev, short events, void* ctx) {
  ClientConnection* client = static_cast<ClientConnection*>(ctx);
  if (client && client->server) {
    client->server->handleClientEvent(client, events);
  }
}

void NetworkServer::handleNewConnection(evutil_socket_t fd, sockaddr* addr,
                                        int socklen) {
  // Create bufferevent for new client
  bufferevent* bev = bufferevent_socket_new(base_, fd, BEV_OPT_CLOSE_ON_FREE);
  if (!bev) {
    std::cerr << "Failed to create bufferevent for new connection" << std::endl;
    evutil_closesocket(fd);
    return;
  }

  // Get client address
  sockaddr_in* client_addr = (sockaddr_in*)addr;
  std::string address;

#ifdef _WIN32
  address = std::string(inet_ntoa(client_addr->sin_addr)) + ":" +
            std::to_string(ntohs(client_addr->sin_port));
#else
  char client_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(client_addr->sin_addr), client_ip, INET_ADDRSTRLEN);
  address = std::string(client_ip) + ":" +
            std::to_string(ntohs(client_addr->sin_port));
#endif

  // Create client connection
  int client_id = next_client_id_++;
  auto client = std::make_unique<ClientConnection>(client_id, bev, address);
  client->server = this;  // Set server reference for callbacks

  // Set callbacks
  bufferevent_setcb(bev, readCallback, nullptr, eventCallback, client.get());
  bufferevent_enable(bev, EV_READ | EV_WRITE);

  // Add to clients list
  {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_.push_back(std::move(client));
    client_count_++;
  }

  std::cout << "Client connected: " << client_id << " from " << address
            << std::endl;

  // Call connect callback
  if (connect_callback_) {
    connect_callback_(client_id);
  }
}

void NetworkServer::handleClientRead(ClientConnection* client) {
  if (!client || !client->bev) {
    return;
  }

  evbuffer* input = bufferevent_get_input(client->bev);
  size_t len = evbuffer_get_length(input);

  if (len > 0) {
    std::vector<uint8_t> data(len);
    evbuffer_remove(input, data.data(), len);

    // Call receive callback
    if (receive_callback_) {
      receive_callback_(client->client_id, data.data(), len);
    }
  }
}

void NetworkServer::handleClientEvent(ClientConnection* client, short events) {
  if (!client) {
    return;
  }

  if (events & BEV_EVENT_ERROR) {
    std::cerr << "Client " << client->client_id << " error: "
              << evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR())
              << std::endl;
  }

  if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
    std::cout << "Client " << client->client_id << " disconnected" << std::endl;

    int client_id = client->client_id;

    // Call disconnect callback
    if (disconnect_callback_) {
      disconnect_callback_(client_id);
    }

    // Remove client
    removeClient(client);
  }
}

void NetworkServer::removeClient(ClientConnection* client) {
  if (!client) {
    return;
  }

  std::lock_guard<std::mutex> lock(clients_mutex_);

  // Find and remove client
  for (auto it = clients_.begin(); it != clients_.end(); ++it) {
    if (it->get() == client) {
      if (client->bev) {
        bufferevent_free(client->bev);
      }
      clients_.erase(it);
      client_count_--;
      break;
    }
  }
}

NetworkServer::ClientConnection* NetworkServer::findClient(int client_id) {
  for (auto& client : clients_) {
    if (client->client_id == client_id) {
      return client.get();
    }
  }
  return nullptr;
}

NetworkServer::ClientConnection* NetworkServer::findClientByBufferevent(
    bufferevent* bev) {
  for (auto& client : clients_) {
    if (client->bev == bev) {
      return client.get();
    }
  }
  return nullptr;
}