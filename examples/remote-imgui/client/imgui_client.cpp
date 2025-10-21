#include "imgui_client.h"

#ifndef DISABLE_NETWORKING
#include "network_client.h"
#include "deserializer.h"
#include "protocol.h"
#endif

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

#ifndef DISABLE_NETWORKING
    // Initialize network client
    network_client_ = std::make_unique<NetworkClient>();
    deserializer_ = std::make_unique<ImDrawDataDeserializer>();
#endif

    // Initialize GLFW
    if (!initGLFW()) {
        return false;
    }

    // Initialize OpenGL
    if (!initOpenGL()) {
        return false;
    }

    // Setup ImGui
    if (!setupImGui()) {
        return false;
    }

    // Set window user pointer and callbacks
    glfwSetWindowUserPointer(window_, this);

    glfwSetKeyCallback(window_, keyCallback);
    glfwSetMouseButtonCallback(window_, mouseButtonCallback);
    glfwSetCursorPosCallback(window_, cursorPosCallback);
    glfwSetScrollCallback(window_, scrollCallback);
    glfwSetWindowSizeCallback(window_, windowSizeCallback);
    glfwSetCharCallback(window_, charCallback);

#ifndef DISABLE_NETWORKING
    // Set network callbacks
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
#else
    // Simplified version: assume connected, for testing rendering
    connected_ = true;
    std::cout << "Network disabled - running in demo mode" << std::endl;
#endif

    initialized_ = true;
    return true;
}

bool ImGuiClient::connect(const std::string& server_ip, int port) {
#ifndef DISABLE_NETWORKING
    if (network_client_) {
        return network_client_->connect(server_ip, std::to_string(port));
    }
#else
    // Simplified version: always return success
    std::cout << "Network disabled - fake connection to " << server_ip << ":" << port << std::endl;
    return true;
#endif
    return false;
}

void ImGuiClient::cleanup() {
#ifndef DISABLE_NETWORKING
    if (network_client_) {
        network_client_->disconnect();
    }
#endif

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

        // Simple delay
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

bool ImGuiClient::shouldClose() const {
    return window_ ? glfwWindowShouldClose(window_) : true;
}

bool ImGuiClient::isConnected() const {
    return connected_;
}

// Private initialization functions - reference imgui/examples/example_glfw_opengl3
bool ImGuiClient::initGLFW() {
    // GLFW error callback
    glfwSetErrorCallback(glfwErrorCallback);

    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // Decide GL+GLSL version
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Create window
    window_ = glfwCreateWindow(window_width_, window_height_, title, NULL, NULL);
    if (!window_) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // Enable vertical sync

    std::cout << "GLFW initialized successfully" << std::endl;
    return true;
}

bool ImGuiClient::initOpenGL() {
    // Initialize GLEW
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(err) << std::endl;
        return false;
    }

    std::cout << "GLEW initialized successfully" << std::endl;
    return true;
}

bool ImGuiClient::setupImGui() {
    // Reference imgui/examples/example_glfw_opengl3 standard initialization flow
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    // Set configuration
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Set style
    ImGui::StyleColorsDark();

    // Set backend
    const char* glsl_version = "#version 130";
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load fonts
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);
    setupFontTexture();

    std::cout << "ImGui setup completed" << std::endl;
    return true;
}

void ImGuiClient::setupFontTexture() {
    // Get font data from serializer and create texture
    // Here simplified handling, should actually receive font texture data from network

    ImGuiIO& io = ImGui::GetIO();

    // Create temporary texture
    glGenTextures(1, &font_texture_);
    glBindTexture(GL_TEXTURE_2D, font_texture_);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Upload font data
    // Should get actual font data from network client
    // Temporarily use placeholder data
    std::vector<unsigned char> temp_font_data(width * height, 0); // Alpha8 format

    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_UNSIGNED_BYTE, temp_font_data.data());

    // Set ImGui font texture
    io.Fonts->TexID = (ImTextureID)(intptr_t)font_texture_;
    io.Fonts->ClearInputData();
    io.Fonts->ClearTexData();

    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "Font texture created: " << font_texture_ << std::endl;
}

void ImGuiClient::renderFrame() {
    // Start new frame
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // Render ImGui interface
    renderImGui();

    // Get draw data and render
#ifndef DISABLE_NETWORKING
    const FrameData* frame_data = deserializer_->getFrameData();
    if (frame_data) {
        renderDrawData(frame_data);
    }
#else
    // Simplified version: render demo interface
    renderDemoInterface();
#endif

    // End new frame
    ImGui::Render();

    // Swap buffers
    glfwSwapBuffers(window_);
}

void ImGuiClient::renderImGui() {
    // Display connection status and information
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

    // Control panel
    {
        ImGui::Text("Press ESC or click Exit button to quit");
    }

    // Usage instructions
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
    // Network events are handled in NetworkClient
    // Can add special event handling logic here
}

#ifndef DISABLE_NETWORKING
void ImGuiClient::renderDrawData(const FrameData* frame_data) {
    // Use ImGui rendering pipeline to render received draw data
    // This requires converting binary data to ImGui draw commands
    // Implement complete rendering logic requires more complex data structure parsing

    // Simplified implementation: directly use ImGui draw data
    // Real implementation should deserialize binary data to ImGui understandable format

    // Temporarily, display a simple status window
    if (frame_data) {
        ImGui::Begin("Remote Frame Data");
        ImGui::Text("Received frame with draw lists");
        ImGui::Text("Display position: %.2f, %.2f",
                   frame_data->header.display_pos[0], frame_data->header.display_pos[1]);
        ImGui::Text("Display size: %.2f, %.2f",
                   frame_data->header.display_size[0], frame_data->header.display_size[1]);
        ImGui::Text("Framebuffer scale: %.2f, %.2f",
                   frame_data->header.framebuffer_scale[0], frame_data->header.framebuffer_scale[1]);
        ImGui::Text("Number of draw lists: %u", frame_data->header.cmd_lists_count);
        ImGui::End();
    }
}
#else
void ImGuiClient::renderDemoInterface() {
    // Simplified version demo interface
    ImGui::ShowDemoWindow();

    ImGui::Begin("Remote ImGui Demo");
    ImGui::Text("This is a demo interface for remote ImGui client");
    ImGui::Text("Network functionality is disabled for testing");

    static float f = 0.0f;
    static int counter = 0;
    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
    if (ImGui::Button("Button")) {
        counter++;
        std::cout << "Button clicked! Counter: " << counter << std::endl;
    }
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);

    ImGui::End();
}
#endif

// GLFW callback function implementations
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
    // Handle character input
    if (initialized_ && connected_) {
        // Can add character input handling
    }
}

// Event sending implementations
void ImGuiClient::sendKeyEvent(int key, int action) {
#ifndef DISABLE_NETWORKING
    if (network_client_ && network_client_->isConnected()) {
        EventPacket event;
        event.event_type = (action == GLFW_PRESS || action == GLFW_REPEAT) ?
            EventType::KEY_PRESS : EventType::KEY_DOWN;
        event.key_code = key;

        // Add window size information
        glfwGetWindowSize(window_, &window_width_, &window_height_);
        event.client_width = window_width_;
        event.client_height = window_height_;

        network_client_->sendEvent(event);
    }
#endif
}

void ImGuiClient::sendMouseEvent(int button, int action, double x, double y) {
#ifndef DISABLE_NETWORKING
    if (network_client_ && network_client_->isConnected()) {
        EventPacket event;
        event.event_type = (action == GLFW_PRESS) ? EventType::MOUSE_DOWN : EventType::MOUSE_UP;
        event.mouse_button = button;
        event.mouse_x = (uint32_t)x;
        event.mouse_y = (uint32_t)y;

        network_client_->sendEvent(event);
    }
#endif
}

void ImGuiClient::sendMouseMoveEvent(double x, double y) {
#ifndef DISABLE_NETWORKING
    if (network_client_ && network_client_->isConnected()) {
        EventPacket event;
        event.event_type = EventType::MOUSE_MOVE;
        event.mouse_x = (uint32_t)x;
        event.mouse_y = (uint32_t)y;

        network_client_->sendEvent(event);
    }
#endif
}

void ImGuiClient::sendScrollEvent(double xoffset, double yoffset) {
#ifndef DISABLE_NETWORKING
    if (network_client_ && network_client_->isConnected()) {
        EventPacket event;
        event.event_type = EventType::MOUSE_WHEEL;
        event.wheel_x = (float)xoffset;
        event.wheel_y = (float)yoffset;

        network_client_->sendEvent(event);
    }
#endif
}

void ImGuiClient::sendResizeEvent(int width, int height) {
#ifndef DISABLE_NETWORKING
    if (network_client_ && network_client_->isConnected()) {
        EventPacket event;
        event.event_type = EventType::RESIZE;
        event.client_width = width;
        event.client_height = height;

        network_client_->sendEvent(event);
    }
#endif
}

} // namespace RemoteImGui