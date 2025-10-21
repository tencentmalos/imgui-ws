#include "imgui_client.h"
#include "network_client.h"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <server_ip> <port>" << std::endl;
        return 1;
    }

    std::string server_ip = argv[1];
    int port = std::atoi(argv[2]);

    std::cout << "Remote ImGui Client" << std::endl;
    std::cout << "Connecting to server: " << server_ip << ":" << port << std::endl;

    RemoteImGui::ImGuiClient client;

    // 初始化客户端
    if (!client.initialize(1280, 720, "Remote ImGui Client")) {
        std::cerr << "Failed to initialize client" << std::endl;
        return 1;
    }

    // 连接到服务器
    if (!client.connect(server_ip, port)) {
        std::cerr << "Failed to connect to server" << std::endl;
        return 1;
    }

    // 运行客户端主循环
    client.run();

    std::cout << "Client shutdown" << std::endl;
    return 0;
}