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

    // Deserialize font texture data
    bool DeserializeFontTexture(const NetPacketBuffer& packet);

    // Individual input event deserialization
    bool DeserializeMouseMoveEvent(const NetPacketBuffer& packet, MouseMoveEvent& event);
    bool DeserializeMouseButtonEvent(const NetPacketBuffer& packet, MouseButtonEvent& event);
    bool DeserializeMouseWheelEvent(const NetPacketBuffer& packet, MouseWheelEvent& event);
    bool DeserializeKeyboardEvent(const NetPacketBuffer& packet, KeyboardEvent& event);
    bool DeserializeCharEvent(const NetPacketBuffer& packet, CharEvent& event);

    // Get deserialized frame data
    const FrameData* GetFrameData() const;

    // Get font texture data
    const FontTextureData* GetFontTextureData() const;

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

    // Parse individual input events
    bool ParseMouseMoveEvent(MouseMoveEvent& event, const std::vector<uint8_t>& data, size_t size, size_t& offset);
    bool ParseMouseButtonEvent(MouseButtonEvent& event, const std::vector<uint8_t>& data, size_t size, size_t& offset);
    bool ParseMouseWheelEvent(MouseWheelEvent& event, const std::vector<uint8_t>& data, size_t size, size_t& offset);
    bool ParseKeyboardEvent(KeyboardEvent& event, const std::vector<uint8_t>& data, size_t size, size_t& offset);
    bool ParseCharEvent(CharEvent& event, const std::vector<uint8_t>& data, size_t size, size_t& offset);

private:
    std::unique_ptr<FrameData> current_frame_;
    std::unique_ptr<FontTextureData> current_font_texture_;
};

}// namespace spatial::debugger