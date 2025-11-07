#pragma once

#include "../net_packet_buffer.hpp"
#include "net_imgui_define.hpp"
#include <vector>
#include <cstdint>

// Forward declarations - ImGui headers will be provided by including projects
struct ImDrawData;
struct ImDrawList;
struct ImDrawCmd;

namespace spatial::debugger {

// Binary serializer with network protocol support
// Reference: imgui-ws implementation, but simplified to avoid complex
// dependencies

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
  NetPacketBuffer getPacketizedData();

  // Font texture serialization
  NetPacketBuffer getFontTexturePacket(uint32_t texture_id, unsigned char* pixels,
                                       int width, int height, uint32_t format = 0);

  // Individual input event serialization
  NetPacketBuffer getMouseMovePacket(const MouseMoveEvent& event);
  NetPacketBuffer getMouseButtonPacket(const MouseButtonEvent& event);
  NetPacketBuffer getMouseWheelPacket(const MouseWheelEvent& event);
  NetPacketBuffer getKeyboardPacket(const KeyboardEvent& event);
  NetPacketBuffer getCharPacket(const CharEvent& event);

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

  // Serialize individual input events
  void serializeMouseMoveEvent(const MouseMoveEvent& event);
  void serializeMouseButtonEvent(const MouseButtonEvent& event);
  void serializeMouseWheelEvent(const MouseWheelEvent& event);
  void serializeKeyboardEvent(const KeyboardEvent& event);
  void serializeCharEvent(const CharEvent& event);
};

}  // namespace spatial::debugger