#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "net_imgui_define.hpp"
#include "net_packet_buffer.hpp"

// Forward declarations - ImGui headers will be provided by including projects
struct ImDrawVert;
using ImDrawIdx = unsigned short;

namespace spatial::debugger {

class ImDrawDataDeserializer {
public:
    ImDrawDataDeserializer();
    ~ImDrawDataDeserializer();

    // Deserialize binary data
    bool DeserializePacket(const NetPacketBuffer& packet);

    // Get deserialized frame data
    const FrameData* GetFrameData() const;

    // Clear data
    void Clear();

    // Has valid frame data
    bool HasValidFrame() const;

private:
    // Parse frame header
    bool ParseFrameHeader(const std::vector<uint8_t>& data, size_t size, size_t& offset);

    // Parse draw lists
    bool ParseDrawLists(const std::vector<uint8_t>& data, size_t size, size_t& offset);

    // Validate data integrity
    bool ValidateData() const;

private:
    std::unique_ptr<FrameData> current_frame_;
};

}// namespace spatial::debugger