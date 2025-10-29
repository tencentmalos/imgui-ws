#pragma once

#include "imgui.h"
#include "network_protocol.h"
#include <vector>
#include <cstdint>

// Binary serializer with network protocol support
// Reference: imgui-ws implementation, but simplified to avoid complex dependencies

struct FrameHeader {
    uint32_t magic;          // Magic number for validation
    uint32_t version;        // Protocol version
    uint32_t total_size;     // Total frame data size
    uint32_t cmd_lists_count; // Number of draw lists
    float display_pos[2];    // Display position
    float display_size[2];   // Display size
    float framebuffer_scale[2]; // Framebuffer scale
};

struct DrawListHeader {
    uint32_t vtx_buffer_size; // Vertex buffer size
    uint32_t idx_buffer_size; // Index buffer size
    uint32_t cmd_count;      // Command count
};

struct DrawCmd {
    uint32_t idx_count;      // Index count
    uint32_t clip_rect[4];   // Clipping rectangle x,y,z,w
    uint32_t texture_id;      // Texture ID
    uint32_t user_callback;   // User callback function pointer (as uint32_t)
    uint32_t user_callback_data_size; // Size of user callback data
    // User callback data follows this structure in the stream
};

class ImDrawDataSerializer {
public:
    ImDrawDataSerializer();
    ~ImDrawDataSerializer();

    // Set ImGui draw data to be serialized
    void setDrawData(ImDrawData* draw_data);

    // Get serialized binary data
    const std::vector<uint8_t>& getSerializedData() const;

    // Clear serialized data
    void clear();

    // Get current frame data size (bytes)
    size_t getDataSize() const;

    // Get packetized data for network transmission
    std::vector<std::vector<uint8_t>> getPacketizedData(uint32_t max_packet_size = MAX_PACKET_SIZE - HEADER_SIZE);

    // Create network packet with header
    static std::vector<uint8_t> createNetworkPacket(
        ServiceType service_type,
        uint32_t service_cmd,
        const uint8_t* data,
        size_t data_size,
        uint32_t packet_id = 0
    );

private:
    std::vector<uint8_t> serialized_data_;
    ImDrawData* current_draw_data_;

    // Serialize single draw list
    void serializeDrawList(const ImDrawList* draw_list);

    // Serialize draw data header
    void serializeHeader(const ImDrawData* draw_data);

    // Convert float to byte array
    void writeFloat(float value);

    // Convert integer to byte array
    void writeUint32(uint32_t value);

    // Write byte data
    void writeBytes(const uint8_t* data, size_t size);
};