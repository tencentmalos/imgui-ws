// Official example-based client for remote ImGui
// Based on Dear ImGui example_glfw_opengl3 with remote functionality

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "deserializer.h"

#include <stdio.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#else
#include <GL/gl.h>
#endif
#include <GLFW/glfw3.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#include <iostream>
#include <vector>
#include <string>

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main(int, char** argv) {
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Remote ImGui Client - Official Example", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If file cannot be loaded, function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    bool show_network_window = true;
    bool show_data_window = true;
    bool show_remote_window = true;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Remote ImGui state
    std::vector<uint8_t> received_data_;
    ImDrawDataDeserializer deserializer_;
    static char server_ip[256] = "127.0.0.1";
    static int server_port = 8080;
    static bool connected = false;

    // Simulate initial connection
    std::cout << "Connecting to " << server_ip << ":" << server_port << " (demo mode)" << std::endl;
    connected = true;

    // Simulate receiving some data
    received_data_.clear();
    for (int i = 0; i < 256; ++i) {
        received_data_.push_back(0x40 + (i % 64));
    }
    std::cout << "Simulated data reception: " << received_data_.size() << " bytes" << std::endl;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // === Remote ImGui Client UI ===

        // Main menu bar
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New")) {}
                if (ImGui::MenuItem("Open", "Ctrl+O")) {}
                ImGui::Separator();
                if (ImGui::MenuItem("Quit", "Alt+F4")) {
                    glfwSetWindowShouldClose(window, true);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Demo Window", NULL, &show_demo_window);
                ImGui::MenuItem("Another Window", NULL, &show_another_window);
                ImGui::MenuItem("Network Settings", NULL, &show_network_window);
                ImGui::MenuItem("Data Viewer", NULL, &show_data_window);
                ImGui::MenuItem("Remote ImGui", NULL, &show_remote_window);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // Remote ImGui main window
        if (show_remote_window) {
            ImGui::Begin("Remote ImGui Client", &show_remote_window);

            ImGui::Text("This is a visual ImGui client for remote rendering");
            ImGui::Text("Based on Dear ImGui official example_glfw_opengl3");

            ImGui::Separator();

            ImGui::Text("Connection Status:");
            ImGui::SameLine();
            if (connected) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Connected to %s:%d", server_ip, server_port);
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Disconnected");
            }

            ImGui::Separator();

            ImGui::Text("Data Reception:");
            ImGui::SameLine();
            ImGui::Text("Received: %zu bytes", received_data_.size());

            if (ImGui::Button("Simulate Data")) {
                static int pattern = 0;
                received_data_.clear();
                for (int i = 0; i < 512; ++i) {
                    received_data_.push_back(0x20 + ((i + pattern) % 96));
                }
                pattern = (pattern + 1) % 4;
                std::cout << "Simulated data reception: " << received_data_.size() << " bytes" << std::endl;
            }

            ImGui::End();
        }

        // Network settings window
        if (show_network_window) {
            ImGui::Begin("Network Settings", &show_network_window);

            ImGui::InputText("Server IP", server_ip, sizeof(server_ip));
            ImGui::InputInt("Server Port", &server_port);

            if (ImGui::Button("Connect")) {
                std::cout << "Connecting to " << server_ip << ":" << server_port << " (demo mode)" << std::endl;
                connected = true;

                // Simulate receiving data
                received_data_.clear();
                for (int i = 0; i < 256; ++i) {
                    received_data_.push_back(0x40 + (i % 64));
                }
                std::cout << "Simulated data reception: " << received_data_.size() << " bytes" << std::endl;
            }
            ImGui::SameLine();
            if (ImGui::Button("Disconnect")) {
                connected = false;
                received_data_.clear();
                std::cout << "Disconnected from server" << std::endl;
            }

            ImGui::Text("Note: This is a demo - no real networking yet");

            ImGui::End();
        }

        // Data viewer window
        if (show_data_window) {
            ImGui::Begin("Data Viewer", &show_data_window);

            ImGui::Text("Received Data Display:");

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

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        if (show_another_window) {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // Main control window
        {
            ImGui::Begin("Main Controls");

            static float f = 0.0f;
            static int counter = 0;

            ImGui::Text("Dear ImGui says hello!");
            ImGui::Checkbox("Demo Window", &show_demo_window);
            ImGui::Checkbox("Another Window", &show_another_window);
            ImGui::Checkbox("Network Settings", &show_network_window);
            ImGui::Checkbox("Data Viewer", &show_data_window);
            ImGui::Checkbox("Remote ImGui", &show_remote_window);

            ImGui::Separator();

            ImGui::SliderFloat("Float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::ColorEdit3("Background Color", (float*)&clear_color); // Edit 3 floats representing a color

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

            ImGui::Separator();

            if (ImGui::Button("Exit Application")) {
                glfwSetWindowShouldClose(window, true);
            }

            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "Application terminated" << std::endl;

    return 0;
}