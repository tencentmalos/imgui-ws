// Simple client based on imgui/examples/example_glfw_opengl3/main.cpp
// This provides proper window creation and rendering

#include "imgui.h"
#include "deserializer.h"
#include <iostream>
#include <vector>
#include <memory>
#include <string>

// Include basic OpenGL headers
#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #include <GL/gl.h>
#else
    #include <GL/gl.h>
#endif

class SimpleImGuiClient {
public:
    SimpleImGuiClient();
    ~SimpleImGuiClient();

    bool initialize(int window_width, int window_height, const char* title);
    void run();
    void cleanup();

    // Connect to server (without real networking for now)
    bool connect(const std::string& server_ip, int port);

private:
    // Window and OpenGL setup
    bool setupWindow();
    bool setupOpenGL();
    void cleanup();

    // ImGui integration
    void setupImGui();

    // Rendering
    void renderFrame();
    void renderImGui();

    // File paths
    std::string getFontPath(const std::string& filename);

    // Members
    GLFWwindow* window_ = nullptr;
    int window_width_ = 1280;
    int window_height_ = 720;
    std::string window_title_;
    bool initialized_ = false;

    // ImGui state
    ImVec4 clear_color_ = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Data reception
    std::vector<uint8_t> received_data_;
};

SimpleImGuiClient::SimpleImGuiClient() {
    window_ = nullptr;
}

SimpleImGuiClient::~SimpleImGuiClient() {
    cleanup();
}

bool SimpleImGuiClient::initialize(int window_width, int window_height, const char* title) {
    window_width_ = window_width;
    window_height_ = window_height;
    window_title_ = title;

    // Setup window and OpenGL
    if (!setupWindow()) {
        return false;
    }
    if (!setupOpenGL()) {
        return false;
    }

    // Setup ImGui
    setupImGui();

    initialized_ = true;
    return true;
}

bool SimpleImGuiClient::connect(const std::string& server_ip, int port) {
    std::cout << "Fake connecting to " << server_ip << ":" << port << " (demo mode)" << std::endl;
    return true;
}

std::string SimpleImGuiClient::getFontPath(const std::string& filename) {
    return std::string("fonts/") + filename;
}

bool SimpleImGuiClient::setupWindow() {
    // Error callback
    glfwSetErrorCallback([](int error, const char* description) {
        std::cerr << "GLFW Error " << error << ": " << description << std::endl;
    });

    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // Window hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    window_ = glfwCreateWindow(window_width_, window_height_, window_title_.c_str(), NULL, NULL);
    if (!window_) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // Enable vsync

    std::cout << "Window created successfully" << std::endl;
    return true;
}

bool SimpleImGuiClient::setupOpenGL() {
    // For demo, we'll use basic OpenGL without complex checks
    return true;
}

void SimpleImGuiClient::cleanup() {
    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    glfwTerminate();
}

void SimpleImGuiClient::setupImGui() {
    // Create ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    // Configuration
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // Style
    ImGui::StyleColorsDark();

    // Setup Dear ImGui binding
    const char* glsl_version = "#version 130";
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    io.Fonts->AddFontDefault();
    io.Fonts->AddFontFromFileTTF(getFontPath("arial.ttf"), 16.0f);
    io.Fonts->AddFontFromFileTTF(getFontPath("Roboto-Medium.ttf"), 16.0f);

    // Build font atlas
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

    // Create OpenGL texture
    GLuint font_texture;
    glGenTextures(1, &font_texture);
    glBindTexture(GL_TEXTURE_2D, font_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload font data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_UNSIGNED_BYTE, pixels);

    // Store texture ID
    io.Fonts->SetTexID((ImTextureID)(intptr_t)font_texture);
    io.Fonts->ClearTexData();

    // Cleanup temporary font data
    io.Fonts->ClearInputData();

    std::cout << "ImGui setup completed" << std::endl;
}

void SimpleImGuiClient::renderFrame() {
    // Start new frame
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // Render ImGui
    renderImGui();

    // End frame
    ImGui::Render();

    // Swap buffers
    glfwSwapBuffers(window_);
}

void SimpleImGuiClient::renderImGui() {
    // Main window
    {
        ImGui::Begin("Remote ImGui Client");

        ImGui::Text("This is a basic ImGui client implementation");
        ImGui::Text("Window: " << window_width_ << "x" << window_height_);

        ImGui::Separator();

        ImGui::ColorEdit3("Background", (float*)&clear_color_);

        ImGui::Text("Data reception:");
        ImGui::SameLine();
        ImGui::Text("Received: " << received_data_.size() << " bytes");

        ImGui::Separator();

        ImGui::Text("Controls:");
        ImGui::Text("- Click to test interaction");
        ImGui::Text("- ESC to exit");

        ImGui::Separator();

        ImGui::Text("Received Data Display:");

        // Display received data in a simple text format
        if (!received_data_.empty()) {
            ImGui::BeginChild("Data View", ImVec2(0, ImGui::GetFrameHeight() - 80), true);

            ImGui::Text("First 64 bytes:");
            size_t display_size = std::min((size_t)64, received_data_.size());
            for (size_t i = 0; i < display_size; ++i) {
                ImGui::SameLine();
                ImGui::Text("0x%02X", received_data_[i] & 0xFF);
            }

            ImGui::Text("Total: " << received_data_.size() << " bytes");

            ImGui::EndChild();
        } else {
            ImGui::Text("No data received yet");
        }

        ImGui::End();
    }

    // Control panel
    {
        ImGui::Begin("Network Settings");

        static char server_ip[256] = "127.0.0.1";
        static int server_port = 8080;

        ImGui::InputText("Server IP", server_ip, sizeof(server_ip));
        ImGui::SameLine();
        ImGui::InputInt("Server Port", &server_port);

        if (ImGui::Button("Connect")) {
            connect(server_ip, server_port);
        }

        ImGui::Text("Note: This is a demo - no real networking implemented");

        ImGui::End();
    }

    // Demo windows (simplified)
    {
        ImGui::Begin("Demo Windows");

        static bool show_demo = false;
        static bool show_another = false;

        ImGui::Checkbox("Demo Window", &show_demo);
        ImGui::Checkbox("Another Window", &show_another);

        if (show_demo) {
            ImGui::Begin("Dear ImGui Demo", &show_demo);
            ImGui::ShowUserGuide();
            ImGui::ShowDemoWindow(&show_demo);
            ImGui::End();
        }

        if (show_another) {
            ImGui::Begin("Another Window", &show_another);
            ImGui::Text("This is another demo window");
            ImGui::Text("You can add your own ImGui code here");
            ImGui::End();
        }

        ImGui::End();
    }

    // Exit button
    {
        ImGui::Separator();
        if (ImGui::Button("Exit")) {
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
        }
    }
}

void SimpleImGuiClient::run() {
    if (!initialized_ || !window_) {
        std::cerr << "Client not initialized!" << std::endl;
        return;
    }

    std::cout << "Starting ImGui client main loop..." << std::endl;
    std::cout << "Press ESC or click Exit button to quit" << std::endl;

    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        renderFrame();
    }
}

// Remove remote dependency stub
#include "network_stub.cpp"