#include "imgui.h"

#include <iostream>
#include <thread>
#include <chrono>

// Simplified version client, directly referencing ImGui examples

int main(int argc, char** argv) {
    std::cout << "Remote ImGui Simple Client" << std::endl;
    std::cout << "This is a minimal test to verify ImGui can be built" << std::endl;

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    // Simulate simple test loop
    bool running = true;
    static float f = 0.0f;
    static int counter = 0;

    std::cout << "Starting ImGui test loop..." << std::endl;

    for (int i = 0; i < 5 && running; ++i) {
        // Start new frame
        ImGui::NewFrame();

        // Create test interface
        {
            ImGui::Begin("Test Window");
            ImGui::Text("This is a simple test client");
            ImGui::Text("Iteration: %d", i + 1);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);

            if (ImGui::Button("Test Button")) {
                counter++;
                std::cout << "Button clicked! Counter: " << counter << std::endl;
            }
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::End();
        }

        // Render ImGui
        ImGui::Render();

        std::cout << "Frame " << (i + 1) << " rendered successfully" << std::endl;

        // Simulate frame rate limit
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Cleanup
    ImGui::DestroyContext();

    std::cout << "Simple client test completed successfully!" << std::endl;
    return 0;
}