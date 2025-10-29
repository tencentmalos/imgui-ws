// SDL2-based client for remote ImGui with actual window rendering
// Based on imgui-ws imgui-extra implementation

#include "imgui.h"
#include "imgui-extra/imgui_impl.h"
#include "deserializer.h"

#include <SDL.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

class SDL2OpenGLClient {
public:
    SDL2OpenGLClient();
    ~SDL2OpenGLClient();

    bool initialize(int window_width, int window_height, const char* title);
    void run();
    void cleanup();

    // Connect to server (without real networking for now)
    bool connect(const std::string& server_ip, int port);

private:
    // SDL window and OpenGL setup
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

    // Window flags
    bool show_demo_window_ = false;
    bool show_another_window_ = false;
    bool show_network_window_ = true;
    bool show_data_window_ = true;

    // Frame counter
    int frame_count_ = 0;
};

SDL2OpenGLClient::SDL2OpenGLClient() {
    window_ = nullptr;
    gl_context_ = nullptr;
    imgui_context_ = nullptr;
}

SDL2OpenGLClient::~SDL2OpenGLClient() {
    cleanup();
}

bool SDL2OpenGLClient::initialize(int window_width, int window_height, const char* title) {
    window_width_ = window_width;
    window_height_ = window_height;
    window_title_ = title;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Pre-initialize ImGui OpenGL settings using imgui-ws helper
    if (!ImGui_PreInit()) {
        std::cerr << "Failed to pre-initialize ImGui" << std::endl;
        return false;
    }

    // Setup window and OpenGL
    if (!setupSDLWindow()) {
        return false;
    }
    if (!setupOpenGL()) {
        return false;
    }

    // Setup ImGui
    setupImGui();

    initialized_ = true;
    std::cout << "SDL2 OpenGL client initialized successfully" << std::endl;
    return true;
}

bool SDL2OpenGLClient::connect(const std::string& server_ip, int port) {
    std::cout << "Connecting to " << server_ip << ":" << port << " (demo mode)" << std::endl;

    // Simulate receiving some data
    received_data_.clear();
    for (int i = 0; i < 256; ++i) {
        received_data_.push_back(0x40 + (i % 64));
    }

    return true;
}

bool SDL2OpenGLClient::setupSDLWindow() {
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

bool SDL2OpenGLClient::setupOpenGL() {
    // For this demo, we'll assume OpenGL setup is successful
    // Check OpenGL version
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    std::cout << "OpenGL Renderer: " << renderer << std::endl;
    std::cout << "OpenGL Version: " << version << std::endl;
    return true;
}

void SDL2OpenGLClient::cleanup() {
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
    std::cout << "SDL2 OpenGL client cleaned up" << std::endl;
}

void SDL2OpenGLClient::setupImGui() {
    // Initialize ImGui using the imgui-ws helper
    imgui_context_ = ImGui_Init(window_, gl_context_);
    if (!imgui_context_) {
        std::cerr << "Failed to initialize ImGui" << std::endl;
        return;
    }

    // Set current context
    ImGui::SetCurrentContext(imgui_context_);

    // Load fonts
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();

    // Create font texture
    ImGui_CreateFontsTexture();

    // Create device objects
    ImGui_CreateDeviceObjects();

    std::cout << "ImGui setup completed with SDL2+OpenGL3 backend" << std::endl;
}

void SDL2OpenGLClient::renderFrame() {
    if (!initialized_ || !window_ || !imgui_context_) {
        std::cerr << "Client not properly initialized!" << std::endl;
        return;
    }

    // Set current context
    ImGui::SetCurrentContext(imgui_context_);

    // Start the Dear ImGui frame
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

void SDL2OpenGLClient::renderImGui() {
    // Main window
    {
        ImGui::Begin("Remote ImGui Client - SDL2 OpenGL Rendering");

        ImGui::Text("This is a SDL2+OpenGL-based ImGui client");
        ImGui::Text("Window size: %dx%d", window_width_, window_height_);

        // Get current window size
        int display_w, display_h;
        SDL_GetWindowSize(window_, &display_w, &display_h);
        ImGui::Text("Current window size: %dx%d", display_w, display_h);

        ImGui::Separator();

        ImGui::ColorEdit3("Background Color", (float*)&clear_color_);

        ImGui::Text("Data reception:");
        ImGui::SameLine();
        ImGui::Text("Received: %zu bytes", received_data_.size());

        ImGui::Separator();

        ImGui::Text("Performance:");
        ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                    1000.0f / io.Framerate, io.Framerate);
        ImGui::Text("Vertices: %d, Indices: %d", io.MetricsRenderVertices, io.MetricsRenderIndices);

        if (frame_count_ % 60 == 0 && frame_count_ > 0) {
            // Print draw data info every 60 frames
            ImDrawData* draw_data = ImGui::GetDrawData();
            if (draw_data && draw_data->Valid) {
                std::cout << "Frame " << frame_count_ << ": "
                          << draw_data->CmdListsCount << " command lists, "
                          << draw_data->TotalVtxCount << " vertices, "
                          << draw_data->TotalIdxCount << " indices" << std::endl;
            }
        }

        ImGui::Separator();

        // Add some interactive elements
        static float rotation = 0.0f;
        ImGui::SliderFloat("Rotation", &rotation, 0.0f, 360.0f);
        rotation += 0.1f;
        if (rotation > 360.0f) rotation = 0.0f;

        static bool show_about = false;
        if (ImGui::Button("Show About")) {
            show_about = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Exit")) {
            // This won't close the window immediately, but we can set a flag
            initialized_ = false;
        }

        if (show_about) {
            ImGui::Begin("About", &show_about);
            ImGui::Text("Remote ImGui Client");
            ImGui::Text("Built with imgui-ws SDL2 backend");
            ImGui::Text("OpenGL Rendering Backend");
            ImGui::Separator();
            ImGui::Text("Controls:");
            ImGui::BulletText("Use menu to toggle windows");
            ImGui::BulletText("Click and drag to interact");
            ImGui::BulletText("Close window to exit");
            ImGui::End();
        }

        ImGui::End();
    }

    // Network settings window
    if (show_network_window_) {
        ImGui::Begin("Network Settings", &show_network_window_);

        static char server_ip[256] = "127.0.0.1";
        static int server_port = 8080;

        ImGui::InputText("Server IP", server_ip, sizeof(server_ip));
        ImGui::InputInt("Server Port", &server_port);

        if (ImGui::Button("Connect")) {
            connect(server_ip, server_port);
        }
        ImGui::SameLine();
        if (ImGui::Button("Disconnect")) {
            std::cout << "Disconnecting from server" << std::endl;
            received_data_.clear();
        }

        ImGui::Separator();

        if (ImGui::Button("Simulate Data Reception")) {
            // Simulate receiving different data patterns
            static int pattern = 0;
            received_data_.clear();
            for (int i = 0; i < 512; ++i) {
                received_data_.push_back(0x20 + ((i + pattern) % 96));
            }
            pattern = (pattern + 1) % 4;
            std::cout << "Simulated data reception: " << received_data_.size() << " bytes" << std::endl;
        }

        ImGui::Text("Note: This is a demo - no real networking implemented yet");

        ImGui::End();
    }

    // Data viewer window
    if (show_data_window_) {
        ImGui::Begin("Data Viewer", &show_data_window_);

        ImGui::Text("Received Data Display:");

        // Display received data in a hex viewer
        if (!received_data_.empty()) {
            if (ImGui::BeginChild("Hex View", ImVec2(0, 200), true)) {
                ImGui::Text("Hex view (first 256 bytes):");

                for (size_t i = 0; i < std::min((size_t)256, received_data_.size()); i += 16) {
                    // Offset
                    ImGui::Text("%04zx: ", i);
                    ImGui::SameLine();

                    // Hex bytes
                    for (int j = 0; j < 16 && (i + j) < received_data_.size(); ++j) {
                        ImGui::Text("%02X ", received_data_[i + j]);
                        ImGui::SameLine();
                    }

                    // Fill remaining space
                    for (int j = (i + 15 < received_data_.size() ? 16 : (received_data_.size() - i)); j < 16; ++j) {
                        ImGui::Text("   ");
                        ImGui::SameLine();
                    }

                    // ASCII representation
                    ImGui::SameLine();
                    ImGui::Text("| ");
                    for (int j = 0; j < 16 && (i + j) < received_data_.size(); ++j) {
                        char c = received_data_[i + j];
                        ImGui::Text("%c", (c >= 32 && c <= 126) ? c : '.');
                        ImGui::SameLine();
                    }
                    ImGui::NewLine();
                }

                ImGui::Text("Total: %zu bytes", received_data_.size());
                ImGui::EndChild();
            }
        } else {
            ImGui::Text("No data received yet");
        }

        ImGui::End();
    }

    // Demo window
    if (show_demo_window_) {
        ImGui::Begin("Demo Window", &show_demo_window_);
        ImGui::Text("This would show ImGui demo content");
        ImGui::Text("Current frame: %d", frame_count_);

        static float f = 0.0f;
        static int counter = 0;
        ImGui::SliderFloat("Float", &f, 0.0f, 1.0f);
        if (ImGui::Button("Button")) {
            counter++;
        }
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::End();
    }

    // Another window
    if (show_another_window_) {
        ImGui::Begin("Another Window", &show_another_window_);
        ImGui::Text("This is another demo window");
        ImGui::Text("You can add your own ImGui code here");

        static float f = 0.0f;
        static int counter = 0;
        ImGui::SliderFloat("Float", &f, 0.0f, 1.0f);
        if (ImGui::Button("Button")) {
            counter++;
        }
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        // Color picker
        static ImVec4 color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
        ImGui::ColorEdit3("Color", (float*)&color);

        ImGui::End();
    }

    // Control window
    {
        ImGui::Begin("Controls");

        ImGui::Checkbox("Demo Window", &show_demo_window_);
        ImGui::Checkbox("Another Window", &show_another_window_);
        ImGui::Checkbox("Network Settings", &show_network_window_);
        ImGui::Checkbox("Data Viewer", &show_data_window_);

        ImGui::Separator();

        ImGui::Text("Frame: %d", frame_count_);

        if (ImGui::Button("Exit Application")) {
            initialized_ = false;
        }

        ImGui::Text("Close window to exit");

        ImGui::End();
    }
}

void SDL2OpenGLClient::run() {
    if (!initialized_ || !window_) {
        std::cerr << "Client not initialized!" << std::endl;
        return;
    }

    std::cout << "Starting SDL2 OpenGL ImGui client..." << std::endl;
    std::cout << "Window created - you should see ImGui interface" << std::endl;
    std::cout << "Close window to quit" << std::endl;

    // Simulate initial connection
    connect("127.0.0.1", 8080);

    // Main loop
    bool done = false;
    while (!done && initialized_) {
        // Handle SDL events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            // Forward to ImGui
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
        frame_count_++;

        // Simulate data reception every 3 seconds
        if (frame_count_ % 180 == 0) {
            // Simulate receiving new data
            static int data_pattern = 0;
            received_data_.clear();
            for (int i = 0; i < 128; ++i) {
                received_data_.push_back(0x30 + ((i + data_pattern) % 48));
            }
            data_pattern = (data_pattern + 1) % 4;
        }
    }

    std::cout << "SDL2 OpenGL client main loop ended" << std::endl;
}

int main(int argc, char** argv) {
    SDL2OpenGLClient client;

    // Initialize client
    if (!client.initialize(1280, 720, "Remote ImGui Client - SDL2 OpenGL Rendering")) {
        std::cerr << "Failed to initialize SDL2 OpenGL client!" << std::endl;
        return -1;
    }

    // Run main loop
    client.run();

    return 0;
}