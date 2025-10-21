#include "imgui.h"
#include "protocol.h"
#include "serializer.h"

#include "common.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>

// 全局服务器实例
struct ServerInstance {
    std::unique_ptr<ImDrawDataSerializer> serializer;
    std::atomic<int> client_count{0};

    // 简化版本 - 基础实现
    void initialize() {
        serializer = std::make_unique<ImDrawDataSerializer>();
        std::cout << "Server initialized" << std::endl;
    }

    void broadcastDrawData() {
        if (serializer) {
            // 简化版本：绘制数据序列化
            std::cout << "Broadcasting draw data to " << client_count.load() << " clients" << std::endl;
        }
    }

    void cleanup() {
        serializer.reset();
        client_count = 0;
        std::cout << "Server cleanup completed" << std::endl;
    }
};

// 全局服务器实例
static ServerInstance g_server;

int main(int argc, char** argv) {
    printf("Usage: %s [port]\n", argv[0]);

    int port = 8080;
    if (argc > 1) port = atoi(argv[1]);

    // 初始化ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    // 设置字体纹理
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

    // 初始化服务器
    g_server.initialize();

    printf("Remote ImGui Server started on port %d\n", port);
    printf("Press Ctrl+C to stop\n");

    // 服务器主循环
    bool running = true;
    static float f = 0.0f;
    static int counter = 0;

    while (running) {
        // 开始新帧
        ImGui::NewFrame();

        // 创建ImGui界面
        {
            ImGui::Begin("Hello, world!");
            ImGui::Text("This is a Remote ImGui Server!");
            ImGui::Text("Connected clients: %d", g_server.client_count.load());
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                       1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);

            static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
            ImGui::ColorEdit3("clear color", (float*)&clear_color);

            if (ImGui::Button("Button")) {
                counter++;
            }
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            static bool show_demo_window = false;
            if (ImGui::Button("Toggle Demo Window")) {
                show_demo_window = !show_demo_window;
            }

            ImGui::End();
        }

        // 演示窗口
        if (show_demo_window) {
            static bool demo_open = true;
            ImGui::ShowDemoWindow(&demo_open);
        }

        // 渲染ImGui
        ImGui::Render();

        // 获取绘制数据
        ImDrawData* draw_data = ImGui::GetDrawData();
        if (draw_data && draw_data->CmdListsCount > 0) {
            // 序列化绘制数据
            g_server.serializer->setDrawData(draw_data);

            // 广播给所有客户端
            g_server.broadcastDrawData();
        }

        // 简单的延迟
        std::this_thread::sleep_for(std::chrono::milliseconds(16));

        // 检查退出条件 (简化版本)
        // 在实际实现中应该处理 Ctrl+C 信号
        if (counter > 1000) { // 简单退出条件
            running = false;
        }
    }

    // 清理
    ImGui::DestroyContext();
    g_server.cleanup();

    printf("Server stopped\n");
    return 0;
}