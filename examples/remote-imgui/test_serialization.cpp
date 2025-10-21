#include "imgui.h"
#include "serializer.h"
#include "deserializer.h"

#include <iostream>
#include <memory>

int main() {
    std::cout << "Testing ImGui Serialization/Deserialization" << std::endl;

    // 初始化 ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.DisplaySize = ImVec2(800.0f, 600.0f);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    // 创建测试数据
    ImGui::NewFrame();

    // 创建一些测试UI元素
    {
        ImGui::Begin("Test Window");
        ImGui::Text("This is a test window for serialization");

        static float f = 0.0f;
        static int counter = 0;

        ImGui::SliderFloat("Float Slider", &f, 0.0f, 1.0f);
        ImGui::ColorEdit3("Color", new float[3]{0.5f, 0.3f, 0.8f});

        if (ImGui::Button("Test Button")) {
            counter++;
            std::cout << "Button clicked " << counter << " times" << std::endl;
        }
        ImGui::SameLine();
        ImGui::Text("Counter: %d", counter);

        ImGui::End();
    }

    // 渲染并获取绘制数据
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();

    if (!draw_data || draw_data->CmdListsCount == 0) {
        std::cerr << "No draw data generated" << std::endl;
        ImGui::DestroyContext();
        return 1;
    }

    std::cout << "Generated draw data: " << draw_data->CmdListsCount << " draw lists" << std::endl;

    // 测试序列化
    ImDrawDataSerializer serializer;
    serializer.setDrawData(draw_data);

    const std::vector<uint8_t>& serialized_data = serializer.getSerializedData();
    std::cout << "Serialized to " << serialized_data.size() << " bytes" << std::endl;

    if (serialized_data.empty()) {
        std::cerr << "Serialization failed - no data generated" << std::endl;
        ImGui::DestroyContext();
        return 1;
    }

    // 测试反序列化
    ImDrawDataDeserializer deserializer;
    bool deserialize_success = deserializer.deserializePacket(
        serialized_data.data(), serialized_data.size());

    if (!deserialize_success) {
        std::cerr << "Deserialization failed" << std::endl;
        ImGui::DestroyContext();
        return 1;
    }

    const FrameData* frame_data = deserializer.getFrameData();
    if (!frame_data) {
        std::cerr << "No frame data after deserialization" << std::endl;
        ImGui::DestroyContext();
        return 1;
    }

    // 验证数据
    std::cout << "Deserialization successful!" << std::endl;
    std::cout << "Original draw lists: " << draw_data->CmdListsCount << std::endl;
    std::cout << "Deserialized draw lists: " << frame_data->header.cmd_lists_count << std::endl;
    std::cout << "Display size: " << frame_data->header.display_size[0]
              << "x" << frame_data->header.display_size[1] << std::endl;
    std::cout << "Total vertices: " << frame_data->vertex_buffers.size() << std::endl;
    std::cout << "Total indices: " << frame_data->index_buffers.size() << std::endl;
    std::cout << "Total draw commands: " << frame_data->draw_commands.size() << std::endl;

    // 验证数据一致性
    bool data_consistent = true;
    if (draw_data->CmdListsCount != frame_data->header.cmd_lists_count) {
        std::cerr << "ERROR: Draw list count mismatch!" << std::endl;
        data_consistent = false;
    }

    size_t original_vertices = 0;
    size_t original_indices = 0;
    size_t original_commands = 0;

    for (int i = 0; i < draw_data->CmdListsCount; i++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[i];
        original_vertices += cmd_list->VtxBuffer.Size;
        original_indices += cmd_list->IdxBuffer.Size;
        original_commands += cmd_list->CmdBuffer.Size;
    }

    if (original_vertices != frame_data->vertex_buffers.size()) {
        std::cerr << "ERROR: Vertex count mismatch! Original: "
                  << original_vertices << ", Deserialized: "
                  << frame_data->vertex_buffers.size() << std::endl;
        data_consistent = false;
    }

    if (original_indices != frame_data->index_buffers.size()) {
        std::cerr << "ERROR: Index count mismatch! Original: "
                  << original_indices << ", Deserialized: "
                  << frame_data->index_buffers.size() << std::endl;
        data_consistent = false;
    }

    if (original_commands != frame_data->draw_commands.size()) {
        std::cerr << "ERROR: Command count mismatch! Original: "
                  << original_commands << ", Deserialized: "
                  << frame_data->draw_commands.size() << std::endl;
        data_consistent = false;
    }

    if (data_consistent) {
        std::cout << "SUCCESS: All data is consistent!" << std::endl;
    } else {
        std::cout << "FAILURE: Data inconsistency detected!" << std::endl;
    }

    // 清理
    ImGui::DestroyContext();

    return data_consistent ? 0 : 1;
}