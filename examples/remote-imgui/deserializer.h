#pragma once

#include "imgui.h"
#include <vector>
#include <cstdint>
#include <memory>

// Frame data structures for deserialization
struct FrameHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t total_size;
    uint32_t cmd_lists_count;
    float display_pos[2];
    float display_size[2];
    float framebuffer_scale[2];
};

struct DrawListHeader {
    uint32_t vtx_buffer_size;
    uint32_t idx_buffer_size;
    uint32_t cmd_count;
};

struct DrawCmd {
    uint32_t idx_count;
    uint32_t clip_rect[4];
    uint32_t texture_id;
    uint32_t user_callback;
    uint32_t user_callback_data_size;
    std::vector<uint8_t> user_callback_data;
};

// Simplified frame data structure
struct FrameData {
    FrameHeader header;
    std::vector<ImDrawVert> vertex_buffers;
    std::vector<ImDrawIdx> index_buffers;
    std::vector<DrawCmd> draw_commands;
    std::vector<size_t> vertex_offsets;
    std::vector<size_t> index_offsets;
    std::vector<size_t> command_offsets;
    std::vector<uint32_t> command_counts;
};

class ImDrawDataDeserializer {
public:
    ImDrawDataDeserializer();
    ~ImDrawDataDeserializer();

    // Deserialize binary data
    bool deserializePacket(const uint8_t* data, size_t size);

    // Get deserialized frame data
    const FrameData* getFrameData() const;

    // Clear data
    void clear();

    // Has valid frame data
    bool hasValidFrame() const;

private:
    std::unique_ptr<FrameData> current_frame_;

    // Parse frame header
    bool parseFrameHeader(const std::vector<uint8_t>& data, size_t size, size_t& offset);

    // Parse draw lists
    bool parseDrawLists(const std::vector<uint8_t>& data, size_t size, size_t& offset);

    // Validate data integrity
    bool validateData() const;
};