#include "imgui.h"
#include <iostream>
#include <vector>

// Simple test without complex serialization
int main() {
    std::cout << "Simple ImGui Data Test" << std::endl;

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.DisplaySize = ImVec2(800.0f, 600.0f);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    // Create test data
    ImGui::NewFrame();

    {
        ImGui::Begin("Test Window");
        ImGui::Text("Testing ImGui data generation");

        static float f = 0.5f;
        ImGui::SliderFloat("Value", &f, 0.0f, 1.0f);

        static ImVec4 color = ImVec4(0.5f, 0.3f, 0.8f, 1.0f);
        ImGui::ColorEdit3("Color", (float*)&color);

        if (ImGui::Button("Test Button")) {
            std::cout << "Button clicked!" << std::endl;
        }

        ImGui::End();
    }

    // Render and get draw data
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();

    if (!draw_data) {
        std::cerr << "No draw data generated" << std::endl;
        ImGui::DestroyContext();
        return 1;
    }

    std::cout << "SUCCESS: Generated ImGui draw data:" << std::endl;
    std::cout << "  - Draw lists: " << draw_data->CmdListsCount << std::endl;
    std::cout << "  - Display size: " << draw_data->DisplaySize.x << "x" << draw_data->DisplaySize.y << std::endl;
    std::cout << "  - Display pos: (" << draw_data->DisplayPos.x << ", " << draw_data->DisplayPos.y << ")" << std::endl;
    std::cout << "  - Framebuffer scale: (" << draw_data->FramebufferScale.x << ", " << draw_data->FramebufferScale.y << ")" << std::endl;

    // Count total vertices and indices
    size_t total_vertices = 0;
    size_t total_indices = 0;
    size_t total_commands = 0;

    for (int i = 0; i < draw_data->CmdListsCount; i++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[i];
        total_vertices += cmd_list->VtxBuffer.Size;
        total_indices += cmd_list->IdxBuffer.Size;
        total_commands += cmd_list->CmdBuffer.Size;
    }

    std::cout << "  - Total vertices: " << total_vertices << std::endl;
    std::cout << "  - Total indices: " << total_indices << std::endl;
    std::cout << "  - Total draw commands: " << total_commands << std::endl;

    // Cleanup
    ImGui::DestroyContext();

    std::cout << "Test completed successfully!" << std::endl;
    return 0;
}