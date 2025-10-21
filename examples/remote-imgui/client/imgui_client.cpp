#include "imgui_client.h"
#include "network_client.h"
#include "deserializer.h"
#include "protocol.h"

#include <iostream>
#include <chrono>
#include <thread>

namespace RemoteImGui {

ImGuiClient::ImGuiClient()
    : window_(nullptr)
    , window_width_(1280)
    , window_height_(720)
    , initialized_(false)
    , connected_(false)
    , clear_color_(ImVec4(0.45f, 0.55f, 0.60f, 1.00f))
    , font_texture_(0) {
}

ImGuiClient::~ImGuiClient() {
    cleanup();
}

bool ImGuiClient::initialize(int window_width, int window_height, const char* title) {
    window_width_ = window_width;
    window_height_ = window_height;

    // 初始化网络客户端
    network_client_ = std::make_unique<NetworkClient>();
    deserializer_ = std::make_unique<ImDrawDataDeserializer>();

    // 初始化GLFW
    if (!initGLFW()) {
        return false;
    }

    // 初始化OpenGL
    if (!initOpenGL()) {
        return false;
    }

    // 设置ImGui
    if (!setupImGui()) {
        return false;
    }

    // 设置窗口用户指针和回调
    glfwSetWindowUserPointer(window_, this);

    glfwSetKeyCallback(window_, keyCallback);
    glfwSetMouseButtonCallback(window_, mouseButtonCallback);
    glfwSetCursorPosCallback(window_, cursorPosCallback);
    glfwSetScrollCallback(window_, scrollCallback);
    glfwSetWindowSizeCallback(window_, windowSizeCallback);
    glfwSetCharCallback(window_, charCallback);

    // 设置网络回调
    network_client_->setConnectCallback([this](bool success) {
        connected_ = success;
        std::cout << "Connected to server: " << (success ? "SUCCESS" : "FAILED") << std::endl;
    });

    network_client_->setReceiveCallback([this](const Packet& packet) {
        if (packet.header.packet_type == PacketType::DRAW_DATA) {
            deserializer_->deserializePacket(packet.serialize().data(), packet.serialize().size());
        } else if (packet.header.packet_type == PacketType::TEXTURE_DATA) {
            std::cout << "Received texture data, size: " << packet.header.data_size << " bytes" << std::endl;
        }
    });

    network_client_->setDisconnectCallback([this] {
        connected_ = false;
        std::cout << "Disconnected from server" << std::endl;
    });

    initialized_ = true;
    return true;
}

bool ImGuiClient::connect(const std::string& server_ip, int port) {
    if (network_client_) {
        return network_client_->connect(server_ip, std::to_string(port));
    }
    return false;
}

void ImGuiClient::cleanup() {
    if (network_client_) {
        network_client_->disconnect();
    }

    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }

    glewTerminate();
    glfwTerminate();

    initialized_ = false;
}

void ImGuiClient::run() {
    if (!initialized_ || !window_) {
        std::cerr << "ImGuiClient not initialized!" << std::endl;
        return;
    }

    std::cout << "Starting client main loop..." << std::endl;
    std::cout << "Press ESC or Exit button to quit" << std::endl;

    while (!shouldClose()) {
        processNetworkEvents();
        glfwPollEvents();

        renderFrame();

        // 简单的延迟
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

bool ImGuiClient::shouldClose() const {
    return window_ ? glfwWindowShouldClose(window_) : true;
}

bool ImGuiClient::isConnected() const {
    return connected_;
}

// 私有的初始化函数 - 参考 imgui/examples/example_glfw_opengl3
bool ImGuiClient::initGLFW() {
    // GLFW 错误回调
    glfwSetErrorCallback(glfwErrorCallback);

    // 初始化GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // 决定GL+GLSL版本
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // 创建窗口
    window_ = glfwCreateWindow(window_width_, window_height_, "Remote ImGui Client", NULL, NULL);
    if (!window_) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // 启用垂直同步

    std::cout << "GLFW initialized successfully" << std::endl;
    return true;
}

bool ImGuiClient::initOpenGL() {
    // 初始化GLEW
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(err) << std::endl;
        return false;
    }

    std::cout << "GLEW initialized successfully" << std::endl;
    return true;
}

bool ImGuiClient::setupImGui() {
    // 参考 imgui/examples/example_glfw_opengl3 的标准初始化流程
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    // 设置配置
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // 设置样式
    ImGui::StyleColorsDark();

    // 设置后端
    const char* glsl_version = "#version 130";
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // 加载字体
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);
    setupFontTexture();

    std::cout << "ImGui setup completed" << std::endl;
    return true;
}

void ImGuiClient::setupFontTexture() {
    // 从 serializer 获取字体数据并创建纹理
    // 这里简化处理，实际应该从网络接收字体纹理数据

    ImGuiIO& io = ImGui::GetIO();

    // 创建临时纹理
    glGenTextures(1, &font_texture_);
    glBindTexture(GL_TEXTURE_2D, font_texture_);

    // 设置纹理参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 上传字体数据
    // 这里应该从网络客户端获取实际的字体数据
    // 暂时使用占位数据
    std::vector<unsigned char> temp_font_data(width * height, 0); // Alpha8 格式

    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_UNSIGNED_BYTE, temp_font_data.data());

    // 设置ImGui字体纹理
    io.Fonts->TexID = (ImTextureID)(intptr_t)font_texture_;
    io.Fonts->ClearInputData();
    io.Fonts->ClearTexData();

    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "Font texture created: " << font_texture_ << std::endl;
}

void ImGuiClient::renderFrame() {
    // 开始新帧
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // 渲染ImGui界面
    renderImGui();

    // 获取绘制数据并渲染
    const FrameData* frame_data = deserializer_->getFrameData();
    if (frame_data) {
        renderDrawData(frame_data);
    }

    // 结束新帧
    ImGui::Render();

    // 交换缓冲区
    glfwSwapBuffers(window_);
}

void ImGuiClient::renderImGui() {
    // 显示连接状态和信息
    {
        ImGui::Begin("Remote ImGui Client");

        if (connected_) {
            ImGui::Text("Status: Connected");
            ImGui::Text("Displaying remote ImGui interface");
        } else {
            ImGui::Text("Status: Disconnected");
            ImGui::Text("Connect to a server to display remote interface");
        }

        ImGui::ColorEdit3("Background", (float*)&clear_color_);

        if (ImGui::Button("Exit")) {
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
        }

        ImGui::End();
    }

    // 控制面板
    {
        ImGui::Text("Press ESC or click Exit button to quit");
    }

    // 使用说明
    {
        ImGui::Begin("Usage Instructions");
        ImGui::BulletText("Connect to server: client.exe <server_ip> <port>");
        ImGui::BulletText("Move mouse and keyboard events are sent to server");
        ImGui::BulletText("Server renders ImGui interface and broadcasts to all clients");
        ImGui::End();
    }

    ImGui::SameLine();
}

void ImGuiClient::processNetworkEvents() {
    // 网络事件在 NetworkClient 中处理
    // 这里可以添加特殊的事件处理逻辑
}

void ImGuiClient::renderDrawData(const FrameData* frame_data) {
    // 使用ImGui的渲染流程来渲染接收到的绘制数据
    // 这需要将二进制数据转换为ImGui的绘制命令
    // 实现完整的渲染逻辑需要更复杂的数据结构解析

    // 简化实现：直接使用 ImGui 的绘制数据
    // 正式实现应该将二进制数据反序列化为 ImGui 可理解的格式

    // 暂时，显示一个简单的状态窗口
    if (frame_data) {
        ImGui::Begin("Remote Frame Data");
        ImGui::Text("Received frame with draw lists");
        ImGui::Text("Display position: %.2f, %.2f",
                   frame_data->display_pos[0], frame_data->display_pos[1]);
        ImGui::Text("Display size: %.2f, %.2f",
                   frame_data->display_size[0], frame_data->display_size[1]);
        ImGui::Text("Framebuffer scale: %.2f, %.2f",
                   frame_data->framebuffer_scale[0], frame_data->framebuffer_scale[1]);
        ImGui::Text("Number of draw lists: %zu", frame_data->draw_lists.size());
        ImGui::End();
    }
}

// GLFW 回调函数实现
void ImGuiClient::glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

void ImGuiClient::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (initialized_ && connected_) {
        sendKeyEvent(key, action);
    }
}

void ImGuiClient::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (initialized_ && connected_) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        sendMouseEvent(button, action, xpos, ypos);
    }
}

void ImGuiClient::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    if (initialized_ && connected_) {
        sendMouseMoveEvent(xpos, ypos);
    }
}

void ImGuiClient::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    if (initialized_ && connected_) {
        sendScrollEvent(xoffset, yoffset);
    }
}

void ImGuiClient::windowSizeCallback(GLFWwindow* window, int width, int height) {
    if (initialized_ && connected_) {
        sendResizeEvent(width, height);
    }
}

void ImGuiClient::charCallback(GLFWwindow* window, unsigned int codepoint) {
    // 处理字符输入
    if (initialized_ && connected_) {
        // 可以添加字符输入处理
    }
}

// 事件发送实现
void ImGuiClient::sendKeyEvent(int key, int action) {
    if (network_client_ && network_client_->isConnected()) {
        EventPacket event;
        event.event_type = (action == GLFW_PRESS || action == GLFW_REPEAT) ?
            EventType::KEY_PRESS : EventType::KEY_DOWN;
        event.key_code = key;

        // 添加窗口大小信息
        glfwGetWindowSize(window_, &window_width_, &window_height_);
        event.client_width = window_width_;
        event.client_height = window_height_;

        network_client_->sendEvent(event);
    }
}

void ImGuiClient::sendMouseEvent(int button, int action, double x, double y) {
    if (network_client_ && network_client_->isConnected()) {
        EventPacket event;
        event.event_type = (action == GLFW_PRESS) ? EventType::MOUSE_DOWN : EventType::MOUSE_UP;
        event.mouse_button = button;
        event.mouse_x = (uint32_t)x;
        event.mouse_y = (uint32_t)y;

        network_client_->sendEvent(event);
    }
}

void ImGuiClient::sendMouseMoveEvent(double x, double y) {
    if (network_client_ && network_client_->isConnected()) {
        EventPacket event;
        event.event_type = EventType::MOUSE_MOVE;
        event.mouse_x = (uint32_t)x;
        event.mouse_y = (uint32_t)y;

        network_client_->sendEvent(event);
    }
}

void ImGuiClient::sendScrollEvent(double xoffset, double yoffset) {
    if (network_client_ && network_client_->isConnected()) {
        EventPacket event;
        event.event_type = EventType::MOUSE_WHEEL;
        event.wheel_x = (float)xoffset;
        event.wheel_y = (float)yoffset;

        network_client_->sendEvent(event);
    }
}

void ImGuiClient::sendResizeEvent(int width, int height) {
    if (network_client_ && network_client_->isConnected()) {
        EventPacket event;
        event.event_type = EventType::RESIZE;
        event.client_width = width;
        event.client_height = height;

        network_client_->sendEvent(event);
    }
}

} // namespace RemoteImGui