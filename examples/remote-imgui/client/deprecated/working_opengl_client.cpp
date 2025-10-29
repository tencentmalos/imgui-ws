// Working OpenGL client for remote ImGui based on official example
// Uses basic window creation with proper ImGui rendering

#include "imgui.h"
#include "deserializer.h"

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

// Use basic OpenGL and Windows API for window creation
#ifdef _WIN32
#include <windows.h>
#include <GL/gl.h>
#pragma comment(lib, "opengl32")
#else
// Fallback for other platforms
#include <GL/gl.h>
#endif

class WorkingOpenGLClient {
public:
    WorkingOpenGLClient();
    ~WorkingOpenGLClient();

    bool initialize(int window_width, int window_height, const char* title);
    void run();
    void cleanup();

    // Connect to server (without real networking for now)
    bool connect(const std::string& server_ip, int port);

private:
    // Window and OpenGL setup
    bool setupWindow();
    bool setupOpenGL();
    bool createGLContext();

    // ImGui integration
    void setupImGui();

    // Basic rendering
    void renderFrame();
    void renderImGui();
    void swapBuffers();

    // Members
#ifdef _WIN32
    HDC hdc_ = nullptr;
    HGLRC hglrc_ = nullptr;
#endif
    void* window_ = nullptr;
    int window_width_ = 1280;
    int window_height_ = 720;
    std::string window_title_;
    bool initialized_ = false;
    bool opengl_initialized_ = false;

    // ImGui state
    ImVec4 clear_color_ = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Data reception
    std::vector<uint8_t> received_data_;

    // Window flags
    bool show_demo_window_ = false;
    bool show_another_window_ = false;
    bool show_network_window_ = true;
    bool show_data_window_ = true;
    bool show_remote_window_ = true;

    // Frame counter
    int frame_count_ = 0;
};

WorkingOpenGLClient::WorkingOpenGLClient() {
    window_ = nullptr;
#ifdef _WIN32
    hdc_ = nullptr;
    hglrc_ = nullptr;
#endif
    opengl_initialized_ = false;
}

WorkingOpenGLClient::~WorkingOpenGLClient() {
    cleanup();
}

bool WorkingOpenGLClient::initialize(int window_width, int window_height, const char* title) {
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
    if (!createGLContext()) {
        return false;
    }

    // Setup ImGui
    setupImGui();

    initialized_ = true;
    std::cout << "OpenGL client initialized successfully" << std::endl;
    return true;
}

bool WorkingOpenGLClient::connect(const std::string& server_ip, int port) {
    std::cout << "Connecting to " << server_ip << ":" << port << " (demo mode)" << std::endl;

    // Simulate receiving some data
    received_data_.clear();
    for (int i = 0; i < 256; ++i) {
        received_data_.push_back(0x40 + (i % 64));
    }

    return true;
}

#ifdef _WIN32
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
#endif

bool WorkingOpenGLClient::setupWindow() {
#ifdef _WIN32
    // Register window class
    WNDCLASSEX wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"WorkingOpenGLClient";
    wc.lpszMenuName = NULL;

    RegisterClassEx(&wc, sizeof(wc), NULL, NULL, NULL);

    // Create window
    DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CAPTION | WS_MINIMIZEBOX;
    RECT rect = {0, 0, window_width_, window_height_};
    window_ = CreateWindowEx(
        WS_EX_OVERLAPPEDWINDOW,
        window_title_.c_str(),
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect,
        NULL,
        NULL,
        GetModuleHandle(NULL),
        wc,
        NULL
    );

    if (!window_) {
        std::cerr << "Failed to create window" << std::endl;
        return false;
    }

    ShowWindow(window_, SW_SHOW);
    UpdateWindow(window_);
    return true;
#else
    // Non-Windows implementation would need GLFW or similar
    std::cerr << "This implementation only supports Windows for now" << std::endl;
    return false;
#endif
}

bool WorkingOpenGLClient::setupOpenGL() {
#ifdef _WIN32
    hdc_ = GetDC(window_);
    if (!hdc_) {
        std::cerr << "Failed to get device context" << std::endl;
        return false;
    }

    // Create OpenGL context
    hglrc_ = wglCreateContext(hdc_, NULL);
    if (!hglrc_) {
        std::cerr << "Failed to create OpenGL context" << std::endl;
        return false;
    }

    // Select pixel format
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        pfd.nVersion = 1,
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        pfd.iPixelType = PFD_TYPE_RGBA,
        pfd.cColorBits = 24,
        pfd.cDepthBits = 24,
        pfd.iLayerType = PFD_MAIN_PLANE
    };

    int iFormat = ChoosePixelFormat(hdc_, &pfd);
    if (iFormat == 0) {
        std::cerr << "Failed to choose pixel format" << std::endl;
        return false;
    }

    if (!SetPixelFormat(hdc_, iFormat, &pfd)) {
        DeleteObject(hglrc_);
        ReleaseDC(hdc_);
        std::cerr << "Failed to set pixel format" << std::endl;
        return false;
    }

    // Create final OpenGL context
    HGLRC temp_hglrc = wglCreateContext(hdc_, NULL);
    if (!temp_hglrc) {
        std::cerr << "Failed to create final OpenGL context" << std::endl;
        return false;
    }

    if (hglrc_) {
        wglDeleteContext(hglrc_);
    }
    hglrc_ = temp_hglrc;

    std::cout << "OpenGL context created successfully" << std::endl;
    return true;
#else
    // For non-Windows, would need to implement with GLFW/SDL
    return false;
#endif
}

bool WorkingOpenGLClient::createGLContext() {
#ifdef _WIN32
    // Store current context
    hdc_ = GetDC(window_);
    if (!hdc_) {
        return false;
    }

    wglMakeCurrent(hdc_, hglrc_);

    // Basic OpenGL setup
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_COLOR_MATERIAL_FACE);
    glEnable(GL_LIGHT0);

    // Basic matrix setup
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Set up perspective projection
    glViewport(0, 0, window_width_, window_height_);
    float aspect = (float)window_width_ / window_height_;
    float nearPlane = 1.0f;
    float farPlane = 100.0f;
    float fovy = 60.0f * 3.14159265358979323846264338327950288419716939937510;
    float top = nearPlane * tanf(fovy * 3.14159f / 360.0f);
    float bottom = nearPlane * tanf(fovy * 3.14159f / 360.0f);
    float yScale = (bottom - top) / (nearPlane - farPlane);

    glFrustum(-aspect * 0.1f, yScale, nearPlane, farPlane, nearPlane, -1.0f, 1.0f);

    opengl_initialized_ = true;
    return true;
#else
    return false;
#endif
}

void WorkingOpenGLClient::setupImGui() {
    // Create ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Configuration
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Style
    ImGui::StyleColorsDark();

    // Basic font setup
    io.Fonts->AddFontDefault();

    std::cout << "ImGui setup completed" << std::endl;
}

void WorkingOpenGLClient::cleanup() {
    if (ImGui::GetCurrentContext()) {
        ImGui::DestroyContext();
    }

#ifdef _WIN32
    if (hglrc_) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(hglrc_);
        hglrc_ = nullptr;
    }
    if (hdc_) {
        ReleaseDC(hdc_);
        hdc_ = nullptr;
    }
    if (window_) {
        DestroyWindow(window_);
        window_ = nullptr;
    }

    DeleteDC(hdc_);
    hdc_ = nullptr;
#else
    // For non-Windows, cleanup would need to handle GLFW/SDL
    glfwTerminate();
#endif

    std::cout << "Client cleaned up" << std::endl;
}

void WorkingOpenGLClient::renderFrame() {
    if (!initialized_) {
        std::cerr << "Client not initialized!" << std::endl;
        return;
    }

    // Start new ImGui frame
    ImGui::NewFrame();

    // Render ImGui
    renderImGui();

    // End frame
    ImGui::Render();

#ifdef _WIN32
    // Rendering
    int display_w, display_h;
    glfwGetWindowSize(window_, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);

    glClearColor(clear_color_.x * clear_color_.w, clear_color_.y * clear_color_.w, clear_color_.z * clear_color_.w, clear_color_.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up perspective matrix for 3D
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float aspect = (float)window_width_ / window_height_;
    float nearPlane = 1.0f;
    float farPlane = 100.0f;
    float fovy = 60.0f * 3.14159f / 360.0f;
    float top = nearPlane * tanf(fovy * 3.14159f / 360.0f);
    float bottom = nearPlane * tanf(fovy * 3.14159f / 360.0f);
    float yScale = (bottom - top) / (nearPlane - farPlane);

    glFrustum(-aspect * 0.1f, yScale, nearPlane, farPlane, nearPlane, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();

    // Basic Dear ImGui rendering
    if (draw_data && draw_data->Valid && draw_data->TotalVtxCount > 0) {
        // Enable client states
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);

        // Set up vertex pointers
        glVertexPointer(2, GL_FLOAT, 0, sizeof(ImDrawVert));
        glColorPointer(4, GL_UNSIGNED_BYTE, 0, sizeof(ImDrawVert));
        glTexCoordPointer(2, GL_FLOAT, 0, sizeof(ImDrawVert));

        // Draw ImGui
        int vtx_offset = 0;
        int idx_offset = 0;
        for (int n = 0; n < draw_data->CmdListsCount; n++) {
            const ImDrawCmd* cmd_list = draw_data->CmdLists[n];
            const ImDrawIdx* idx_buffer = &draw_data->IdxBuffer[idx_offset];
            const ImDrawVert* vtx_buffer = &draw_data->VtxBuffer[vtx_offset];

            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
                if (vtx_buffer && idx_buffer) {
                    glDrawElements(GL_TRIANGLES,
                        idx_buffer[cmd_i].IdxCount,
                        GL_UNSIGNED_INT,
                        idx_offset + cmd_i->IdxOffset);
                    );
                }
            vtx_offset += cmd_list->VtxOffset;
            idx_offset += cmd_list->IdxOffset;
        }
        }
    }

    // Disable client states
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#else
    // For non-Windows, would need proper OpenGL context rendering
    if (draw_data && draw_data->Valid) {
        // Basic triangle for demonstration
        glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.0f, 1.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(0.5f, -0.5f, 0.0f, 0.0f);
        glVertex3f(-0.5f, 0.5f, 0.0f, 0.0f);
        glEnd();
    }
#endif

    swapBuffers();
}

void WorkingOpenGLClient::renderImGui() {
    // Main menu bar
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New")) {}
            if (ImGui::MenuItem("Open", "Ctrl+O")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
#ifdef _WIN32
                PostQuitMessage(window_, WM_QUIT, 0, 0);
#else
                initialized_ = false;
#endif
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Demo Window", NULL, &show_demo_window_);
            ImGui::MenuItem("Another Window", NULL, &show_another_window_);
            ImGui::MenuItem("Network Settings", NULL, &show_network_window_);
            ImGui::MenuItem("Data Viewer", NULL, &show_data_window_);
            ImGui::MenuItem("Remote ImGui", NULL, &show_remote_window_);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    // Remote ImGui main window
    if (show_remote_window) {
        ImGui::Begin("Remote ImGui Client - Real Window", &show_remote_window);

        ImGui::Text("This is a real window-based OpenGL ImGui client");
        ImGui::Text("Window size: %dx%d", window_width_, window_height_);

        ImGui::Separator();

        ImGui::Text("Data reception:");
        ImGui::SameLine();
        ImGui::Text("Received: %zu bytes", received_data_.size());

        ImGui::Separator();

        ImGui::Text("Performance:");
        ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                    1000.0f / io.Framerate, io.Framerate);

        if (frame_count_ % 60 == 0 && frame_count_ > 0) {
            // Print some OpenGL info every 60 frames
            std::cout << "Frame " << frame_count_ << ": OpenGL ready, ImGui active" << std::endl;
        }

        ImGui::Separator();

        ImGui::ColorEdit3("Background Color", (float*)&clear_color_);

        ImGui::Text("Controls:");
        if (ImGui::Button("Simulate Data")) {
            // Simulate receiving different data patterns
            static int pattern = 0;
            received_data_.clear();
            for (int i = 0; i < 512; ++i) {
                received_data_.push_back(0x20 + ((i + pattern) % 96));
            }
            pattern = (pattern + 1) % 4;
            std::cout << "Simulated data reception: " << received_data_.size() << " bytes" << std::endl;
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear Data")) {
            received_data_.clear();
            std::cout << "Data cleared" << std::endl;
        }

        ImGui::Text("Close window to exit");

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
            for (int i = 0; i < 256; ++i) {
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

        ImGui::ColorPicker3("Color", (float*)&clear_color_);
        ImGui::End();
    }

    // Control window
    {
        ImGui::Begin("Controls");

        ImGui::Checkbox("Demo Window", &show_demo_window_);
        ImGui::Checkbox("Another Window", &show_another_window_);
        ImGui::Checkbox("Network Settings", &show_network_window_);
        ImGui::Checkbox("Data Viewer", &show_data_window_);
        ImGui::Checkbox("Remote ImGui", &show_remote_window_);

        ImGui::Separator();

        ImGui::Text("Frame: %d", frame_count_);

        ImGui::Text("OpenGL Info:");
        ImGui::SameLine();
        ImGui::Text("Window: %dx%d", window_width_, window_height_);
        ImGui::SameLine();
        ImGui::Text("Initialized: %s", initialized_ ? "Yes" : "No");

        ImGui::Text("OpenGL Ready: %s", opengl_initialized_ ? "Yes" : "No");

        ImGui::Separator();

        if (ImGui::Button("Clear Data")) {
            received_data_.clear();
            std::cout << "Data cleared" << std::endl;
        }

        ImGui::Text("Exit Application");

        ImGui::End();
    }

    // Increment frame counter
    frame_count_++;
}

void WorkingOpenGLClient::swapBuffers() {
#ifdef _WIN32
    SwapBuffers(hdc_);
#else
    // For non-Windows, would need glfwSwapBuffers or similar
#endif
}

void WorkingOpenGLClient::run() {
    if (!initialized_) {
        std::cerr << "Client not initialized!" << std::endl;
        return;
    }

    std::cout << "Starting real window OpenGL ImGui client..." << std::endl;
    std::cout << "Window created - you should see ImGui interface" << std::endl;
    std::cout << "Close window to quit" << std::endl;

    // Simulate initial connection
    connect("127.0.0.1", 8080);

    // Main loop
#ifdef _WIN32
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0, NULL) > 0) {
        if (msg.message == WM_QUIT ||
            (msg.message == WM_CLOSE && msg.lParam == 0)) {
            break;
        }

        // Forward messages to ImGui if needed
        TranslateMessage(&msg);

        // Render frame
        renderFrame();

        // Update frame counter
        frame_count_++;
    }
#else
    // For non-Windows, would need a proper event loop
    bool done = false;
    while (!done) {
        renderFrame();
        // Check for exit conditions
        done = false; // Would normally check window close, etc.

        frame_count_++;
    }
#endif

    std::cout << "Real OpenGL client main loop ended" << std::endl;
}

int main(int, char** argv) {
    WorkingOpenGLClient client;

    // Initialize client
    if (!client.initialize(1280, 720, "Remote ImGui Client - Real Window")) {
        std::cerr << "Failed to initialize real OpenGL client!" << std::endl;
        return -1;
    }

    // Run main loop
    client.run();

    return 0;
}