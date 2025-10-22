// Simple SDL2-based client for remote ImGui
// Based on imgui-ws imgui_impl structure

#include "imgui.h"
#include "deserializer.h"
#include "imgui-extra/imgui_impl.h"

#include <SDL.h>
#include <iostream>
#include <vector>
#include <memory>
#include <string>

class SimpleSDL2Client {
public:
    SimpleSDL2Client();
    ~SimpleSDL2Client();

    bool initialize(int window_width, int window_height, const char* title);
    void run();
    void cleanup();

    // Connect to server (without real networking for now)
    bool connect(const std::string& server_ip, int port);

private:
    // SDL2 window and OpenGL setup
    bool setupSDLWindow();
    bool setupOpenGL();

    // ImGui integration
    void setupImGui();

    // Rendering
    void renderFrame();
    void renderImGui();

    // Members
    SDL_Window* window_ = nullptr;
    SDL_GLContext gl_context_ = nullptr;
    int window_width_ = 1280;
    int window_height_ = 720;
    std::string window_title_;
    bool initialized_ = false;
    ImGuiContext* imgui_context_ = nullptr;

    // ImGui state
    ImVec4 clear_color_ = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Data reception
    std::vector<uint8_t> received_data_;
    ImDrawDataDeserializer deserializer_;
};

SimpleSDL2Client::SimpleSDL2Client() {
    window_ = nullptr;
    gl_context_ = nullptr;
    imgui_context_ = nullptr;
}

SimpleSDL2Client::~SimpleSDL2Client() {
    cleanup();
}

bool SimpleSDL2Client::initialize(int window_width, int window_height, const char* title) {
    window_width_ = window_width;
    window_height_ = window_height;
    window_title_ = title;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Pre-initialize ImGui OpenGL settings
    if (!ImGui_PreInit()) {
        std::cerr << "Failed to pre-initialize ImGui" << std::endl;
        return false;
    }

    // Setup SDL window and OpenGL
    if (!setupSDLWindow()) {
        return false;
    }
    if (!setupOpenGL()) {
        return false;
    }

    // Setup ImGui
    setupImGui();

    initialized_ = true;
    std::cout << "SDL2 client initialized successfully" << std::endl;
    return true;
}

bool SimpleSDL2Client::connect(const std::string& server_ip, int port) {
    std::cout << "Fake connecting to " << server_ip << ":" << port << " (demo mode)" << std::endl;
    return true;
}

bool SimpleSDL2Client::setupSDLWindow() {
    // Create window with graphics context
    window_ = SDL_CreateWindow(
        window_title_.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        window_width_,
        window_height_,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );

    if (!window_) {
        std::cerr << "Failed to create SDL window: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create OpenGL context
    gl_context_ = SDL_GL_CreateContext(window_);
    if (!gl_context_) {
        std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window_);
        return false;
    }

    // Enable vsync
    SDL_GL_SetSwapInterval(1);

    std::cout << "SDL window created successfully" << std::endl;
    return true;
}

bool SimpleSDL2Client::setupOpenGL() {
    // For demo, we'll use basic OpenGL without complex checks
    // In a real implementation, you might want to check OpenGL version
    return true;
}

void SimpleSDL2Client::cleanup() {
    if (imgui_context_) {
        ImGui_Shutdown();
        ImGui::DestroyContext(imgui_context_);
        imgui_context_ = nullptr;
    }

    if (gl_context_) {
        SDL_GL_DeleteContext(gl_context_);
        gl_context_ = nullptr;
    }

    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    SDL_Quit();
    std::cout << "SDL2 client cleaned up" << std::endl;
}

void SimpleSDL2Client::setupImGui() {
    // Initialize ImGui using the imgui_impl structure
    imgui_context_ = ImGui_Init(window_, gl_context_);
    if (!imgui_context_) {
        std::cerr << "Failed to initialize ImGui" << std::endl;
        return;
    }

    // Set current context
    ImGui::SetCurrentContext(imgui_context_);

    // Load fonts (basic setup)
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();

    // Build font atlas
    ImGui_CreateFontsTexture();

    std::cout << "ImGui setup completed" << std::endl;
}

void SimpleSDL2Client::renderFrame() {
    if (!initialized_ || !window_ || !imgui_context_) {
        std::cerr << "Client not properly initialized!" << std::endl;
        return;
    }

    // Set current context
    ImGui::SetCurrentContext(imgui_context_);

    // Start new frame
    ImGui_NewFrame(window_);

    // Render ImGui
    renderImGui();

    // End frame
    ImGui::Render();

    // Get draw data and render
    ImDrawData* draw_data = ImGui::GetDrawData();
    ImGui_RenderDrawData(draw_data);

    // Swap buffers
    SDL_GL_SwapWindow(window_);
}

void SimpleSDL2Client::renderImGui() {
    // Main window
    {
        ImGui::Begin("Remote ImGui Client");

        ImGui::Text("This is a basic ImGui client implementation using SDL2");
        ImGui::Text("Window: %dx%d", window_width_, window_height_);

        ImGui::Separator();

        ImGui::ColorEdit3("Background", (float*)&clear_color_);

        ImGui::Text("Data reception:");
        ImGui::SameLine();
        ImGui::Text("Received: %zu bytes", received_data_.size());

        ImGui::Separator();

        ImGui::Text("Controls:");
        ImGui::Text("- Click to test interaction");
        ImGui::Text("- ESC or close window to exit");

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

            ImGui::Text("Total: %zu bytes", received_data_.size());

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

        ImGui::Text("Note: This is a demo - no real networking implemented yet");

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
            // Note: ImGui::ShowDemoWindow() would require imgui_demo.cpp
            ImGui::Text("Demo window would show here");
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
}

void SimpleSDL2Client::run() {
    if (!initialized_ || !window_) {
        std::cerr << "Client not initialized!" << std::endl;
        return;
    }

    std::cout << "Starting SDL2 ImGui client main loop..." << std::endl;
    std::cout << "Press ESC or close window to quit" << std::endl;

    bool done = false;
    while (!done) {
        // Handle SDL events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            // Forward to ImGui
            ImGui::SetCurrentContext(imgui_context_);
            ImGui_ProcessEvent(&event);

            // Handle exit
            if (event.type == SDL_QUIT) {
                done = true;
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                done = true;
            }
        }

        renderFrame();
    }

    std::cout << "SDL2 client main loop ended" << std::endl;
}

int main(int argc, char** argv) {
    SimpleSDL2Client client;

    // Initialize client
    if (!client.initialize(1280, 720, "Remote ImGui SDL2 Client")) {
        std::cerr << "Failed to initialize client!" << std::endl;
        return -1;
    }

    // Fake connect to server
    client.connect("127.0.0.1", 8080);

    // Run main loop
    client.run();

    return 0;
}