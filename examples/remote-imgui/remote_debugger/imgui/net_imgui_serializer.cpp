// Include ImGui headers first to resolve type definitions
#include "imgui.h"

#include "net_imgui_serializer.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>

#include "../net_packet_encoder.hpp"

namespace spatial::debugger {

ImDrawDataSerializer::ImDrawDataSerializer() : current_draw_data_(nullptr) {
  serialized_data_.reserve(1024 * 1024);  // Pre-allocate 1MB
}

ImDrawDataSerializer::~ImDrawDataSerializer() { clear(); }

void ImDrawDataSerializer::setDrawData(ImDrawData* draw_data) {
  current_draw_data_ = draw_data;
  serialized_data_.clear();

  if (!draw_data || draw_data->CmdListsCount == 0) {
    return;
  }

  // Serialize header information
  serializeHeader(draw_data);

  // Serialize each draw list
  for (int n = 0; n < draw_data->CmdListsCount; n++) {
    const ImDrawList* cmd_list = draw_data->CmdLists[n];
    serializeDrawList(cmd_list);
  }

  // Update total size in header
  if (serialized_data_.size() >= sizeof(FrameHeader)) {
    *reinterpret_cast<uint32_t*>(&serialized_data_[8]) =
        static_cast<uint32_t>(serialized_data_.size());
  }

  std::cout << "Serialized ImGui frame: " << draw_data->CmdListsCount
            << " draw lists, total size: " << serialized_data_.size()
            << " bytes" << std::endl;
}

void ImDrawDataSerializer::serializeHeader(const ImDrawData* draw_data) {
  // Write frame header
  writeUint32(IMGUI_FRAME_MAGIC);       //magic
  writeUint32(IMGUI_PROTOCOL_VERSION);       //version
  writeUint32(0);  // total_size, will be updated later
  writeUint32(draw_data->CmdListsCount);

  writeFloat(draw_data->DisplayPos.x);
  writeFloat(draw_data->DisplayPos.y);
  writeFloat(draw_data->DisplaySize.x);
  writeFloat(draw_data->DisplaySize.y);
  writeFloat(draw_data->FramebufferScale.x);
  writeFloat(draw_data->FramebufferScale.y);
}

void ImDrawDataSerializer::serializeDrawList(const ImDrawList* draw_list) {
  // Write draw list header
  DrawListHeader list_header;
  list_header.vtx_buffer_size = draw_list->VtxBuffer.Size * sizeof(ImDrawVert);
  list_header.idx_buffer_size = draw_list->IdxBuffer.Size * sizeof(ImDrawIdx);
  list_header.cmd_count = draw_list->CmdBuffer.Size;

  // Write header as a single block
  writeBytes(reinterpret_cast<const uint8_t*>(&list_header), sizeof(DrawListHeader));

  // Write vertex buffer as a single block
  if (draw_list->VtxBuffer.Size > 0) {
    writeBytes(reinterpret_cast<const uint8_t*>(draw_list->VtxBuffer.Data),
               draw_list->VtxBuffer.Size * sizeof(ImDrawVert));
  }

  // Write index buffer as a single block
  if (draw_list->IdxBuffer.Size > 0) {
    writeBytes(reinterpret_cast<const uint8_t*>(draw_list->IdxBuffer.Data),
               draw_list->IdxBuffer.Size * sizeof(ImDrawIdx));
  }

  // Write draw commands
  for (int i = 0; i < draw_list->CmdBuffer.Size; i++) {
    const ImDrawCmd& cmd = draw_list->CmdBuffer[i];

    DrawCmd serialized_cmd;
    serialized_cmd.idx_count = cmd.ElemCount;
    serialized_cmd.clip_rect[0] = static_cast<uint32_t>(
        cmd.ClipRect.x * 1000.0f);  // Convert to integer storage
    serialized_cmd.clip_rect[1] =
        static_cast<uint32_t>(cmd.ClipRect.y * 1000.0f);
    serialized_cmd.clip_rect[2] =
        static_cast<uint32_t>(cmd.ClipRect.z * 1000.0f);
    serialized_cmd.clip_rect[3] =
        static_cast<uint32_t>(cmd.ClipRect.w * 1000.0f);

    // In cmdlist-based organization, offsets are relative to the current cmdlist
    // These offsets should be used as-is since they refer to positions within this cmdlist's buffers
    serialized_cmd.vtx_offset = static_cast<uint32_t>(cmd.VtxOffset);
    serialized_cmd.idx_offset = static_cast<uint32_t>(cmd.IdxOffset);

    // Map texture IDs: use a fixed ID for font texture, pass through other textures
    ImTextureID tex_id = cmd.GetTexID();
    ImGuiIO& io = ImGui::GetIO();

    if (tex_id == io.Fonts->TexID) {
        // This is the font texture, use fixed ID 1
        serialized_cmd.texture_id = 1;
    } else {
        // Pass through other texture IDs (may need mapping in the future)
        serialized_cmd.texture_id = reinterpret_cast<uintptr_t>(tex_id);
    }

    // Handle UserCallback - note: we can't serialize function pointers directly
    // For now, we'll store a null pointer and handle this in the future if
    // needed
    serialized_cmd.user_callback = 0;
    serialized_cmd.user_callback_data_size = 0;

    // Special handling for UserCallback commands
    if (cmd.UserCallback != nullptr) {
      // For now, we'll mark this as a special callback type
      serialized_cmd.user_callback = 1;  // Special marker for UserCallback

      // Try to serialize some basic callback data if available
      if (cmd.UserCallbackData != nullptr) {
        // For basic rendering callbacks, the data might be simple
        // We'll serialize up to 64 bytes of callback data
        const uint8_t* callback_data =
            reinterpret_cast<const uint8_t*>(cmd.UserCallbackData);
        size_t data_size = 64;  // Fixed size for simplicity

        serialized_cmd.user_callback_data_size =
            static_cast<uint32_t>(data_size);
      }
    }

    // Write DrawCmd structure (including the new offset fields, except the vector part)
    writeBytes(reinterpret_cast<const uint8_t*>(&serialized_cmd),
             offsetof(DrawCmd, user_callback_data) - offsetof(DrawCmd, idx_count));

    // Write user callback data if present
    if (!serialized_cmd.user_callback_data.empty()) {
      writeBytes(serialized_cmd.user_callback_data.data(),
               serialized_cmd.user_callback_data.size());
    }
  }
}

void ImDrawDataSerializer::writeFloat(float value) {
  writeBytes(reinterpret_cast<const uint8_t*>(&value), sizeof(float));
}

void ImDrawDataSerializer::writeUint32(uint32_t value) {
  writeBytes(reinterpret_cast<const uint8_t*>(&value), sizeof(uint32_t));
}

void ImDrawDataSerializer::writeBytes(const uint8_t* data, size_t size) {
  serialized_data_.insert(serialized_data_.end(), data, data + size);
}

const std::vector<uint8_t>& ImDrawDataSerializer::getSerializedData() const {
  return serialized_data_;
}

void ImDrawDataSerializer::clear() {
  serialized_data_.clear();
  current_draw_data_ = nullptr;
}

size_t ImDrawDataSerializer::getDataSize() const {
  return serialized_data_.size();
}

NetPacketBuffer ImDrawDataSerializer::getPacketizedData() {
  NetPacketBuffer packet;

  auto header = NetPacketEncoder::CreaterPacketHeader(NetServiceType::RemoteImgui,
                                        (uint16_t)ImGuiCommand::FrameData,
                                        serialized_data_.size());

  packet.Initialize(header, serialized_data_.data(), serialized_data_.size());

  return packet;
}

NetPacketBuffer ImDrawDataSerializer::getFontTexturePacket(uint32_t texture_id,
                                                           unsigned char* pixels,
                                                           int width, int height,
                                                           uint32_t format) {
  NetPacketBuffer packet;

  if (!pixels || width <= 0 || height <= 0) {
    // Return empty packet for invalid data
    auto header = NetPacketEncoder::CreaterPacketHeader(NetServiceType::RemoteImgui,
                                          (uint16_t)ImGuiCommand::FontTexture,
                                          0);
    packet.Initialize(header, nullptr, 0);
    return packet;
  }

  // Create font texture data
  FontTextureData texture_data;
  texture_data.header.texture_id = texture_id;
  texture_data.header.width = width;
  texture_data.header.height = height;
  texture_data.header.format = format;

  size_t data_size = width * height * (format == 1 ? 4 : 1); // RGBA32 vs Alpha8
  texture_data.header.data_size = data_size;
  texture_data.pixel_data.resize(data_size);

  // Copy pixel data
  memcpy(texture_data.pixel_data.data(), pixels, data_size);

  // Serialize the font texture data
  std::vector<uint8_t> serialized_texture;
  serialized_texture.reserve(sizeof(FontTextureHeader) + data_size);

  // Write header
  serialized_texture.insert(serialized_texture.end(),
                           reinterpret_cast<const uint8_t*>(&texture_data.header),
                           reinterpret_cast<const uint8_t*>(&texture_data.header) + sizeof(FontTextureHeader));

  // Write pixel data
  serialized_texture.insert(serialized_texture.end(),
                           texture_data.pixel_data.begin(),
                           texture_data.pixel_data.end());

  // Create packet
  auto header = NetPacketEncoder::CreaterPacketHeader(NetServiceType::RemoteImgui,
                                        (uint16_t)ImGuiCommand::FontTexture,
                                        serialized_texture.size());

  packet.Initialize(header, serialized_texture.data(), serialized_texture.size());

  std::cout << "Serialized font texture: " << width << "x" << height
            << ", format: " << format << ", data size: " << data_size << " bytes" << std::endl;

  return packet;
}

// Individual input event serialization implementations
NetPacketBuffer ImDrawDataSerializer::getMouseMovePacket(const MouseMoveEvent& event) {
    NetPacketBuffer packet;
    serialized_data_.clear();

    serializeMouseMoveEvent(event);

    auto header = NetPacketEncoder::CreaterPacketHeader(NetServiceType::RemoteImgui,
                                        (uint16_t)ImGuiCommand::MouseMoveEvent,
                                        serialized_data_.size());

    packet.Initialize(header, serialized_data_.data(), serialized_data_.size());

    return packet;
}

NetPacketBuffer ImDrawDataSerializer::getMouseButtonPacket(const MouseButtonEvent& event) {
    NetPacketBuffer packet;
    serialized_data_.clear();

    serializeMouseButtonEvent(event);

    auto header = NetPacketEncoder::CreaterPacketHeader(NetServiceType::RemoteImgui,
                                        (uint16_t)ImGuiCommand::MouseButtonEvent,
                                        serialized_data_.size());

    packet.Initialize(header, serialized_data_.data(), serialized_data_.size());

    return packet;
}

NetPacketBuffer ImDrawDataSerializer::getMouseWheelPacket(const MouseWheelEvent& event) {
    NetPacketBuffer packet;
    serialized_data_.clear();

    serializeMouseWheelEvent(event);

    auto header = NetPacketEncoder::CreaterPacketHeader(NetServiceType::RemoteImgui,
                                        (uint16_t)ImGuiCommand::MouseWheelEvent,
                                        serialized_data_.size());

    packet.Initialize(header, serialized_data_.data(), serialized_data_.size());

    return packet;
}

NetPacketBuffer ImDrawDataSerializer::getKeyboardPacket(const KeyboardEvent& event) {
    NetPacketBuffer packet;
    serialized_data_.clear();

    serializeKeyboardEvent(event);

    auto header = NetPacketEncoder::CreaterPacketHeader(NetServiceType::RemoteImgui,
                                        (uint16_t)ImGuiCommand::KeyboardEvent,
                                        serialized_data_.size());

    packet.Initialize(header, serialized_data_.data(), serialized_data_.size());

    return packet;
}

NetPacketBuffer ImDrawDataSerializer::getCharPacket(const CharEvent& event) {
    NetPacketBuffer packet;
    serialized_data_.clear();

    serializeCharEvent(event);

    auto header = NetPacketEncoder::CreaterPacketHeader(NetServiceType::RemoteImgui,
                                        (uint16_t)ImGuiCommand::CharEvent,
                                        serialized_data_.size());

    packet.Initialize(header, serialized_data_.data(), serialized_data_.size());

    return packet;
}

void ImDrawDataSerializer::serializeMouseMoveEvent(const MouseMoveEvent& event) {
    writeBytes(reinterpret_cast<const uint8_t*>(&event.x), sizeof(event.x));
    writeBytes(reinterpret_cast<const uint8_t*>(&event.y), sizeof(event.y));
}

void ImDrawDataSerializer::serializeMouseButtonEvent(const MouseButtonEvent& event) {
    writeBytes(reinterpret_cast<const uint8_t*>(&event.button), sizeof(event.button));
    writeBytes(reinterpret_cast<const uint8_t*>(&event.action), sizeof(event.action));
    writeBytes(reinterpret_cast<const uint8_t*>(&event.x), sizeof(event.x));
    writeBytes(reinterpret_cast<const uint8_t*>(&event.y), sizeof(event.y));
    writeBytes(reinterpret_cast<const uint8_t*>(&event.shift_pressed), sizeof(event.shift_pressed));
    writeBytes(reinterpret_cast<const uint8_t*>(&event.ctrl_pressed), sizeof(event.ctrl_pressed));
    writeBytes(reinterpret_cast<const uint8_t*>(&event.alt_pressed), sizeof(event.alt_pressed));
    writeBytes(reinterpret_cast<const uint8_t*>(&event.super_pressed), sizeof(event.super_pressed));
}

void ImDrawDataSerializer::serializeMouseWheelEvent(const MouseWheelEvent& event) {
    writeBytes(reinterpret_cast<const uint8_t*>(&event.x_offset), sizeof(event.x_offset));
    writeBytes(reinterpret_cast<const uint8_t*>(&event.y_offset), sizeof(event.y_offset));
    writeBytes(reinterpret_cast<const uint8_t*>(&event.mouse_x), sizeof(event.mouse_x));
    writeBytes(reinterpret_cast<const uint8_t*>(&event.mouse_y), sizeof(event.mouse_y));
}

void ImDrawDataSerializer::serializeKeyboardEvent(const KeyboardEvent& event) {
    writeUint32(event.key_code);
    writeBytes(reinterpret_cast<const uint8_t*>(&event.action), sizeof(event.action));
    writeBytes(reinterpret_cast<const uint8_t*>(&event.shift_pressed), sizeof(event.shift_pressed));
    writeBytes(reinterpret_cast<const uint8_t*>(&event.ctrl_pressed), sizeof(event.ctrl_pressed));
    writeBytes(reinterpret_cast<const uint8_t*>(&event.alt_pressed), sizeof(event.alt_pressed));
    writeBytes(reinterpret_cast<const uint8_t*>(&event.super_pressed), sizeof(event.super_pressed));
}

void ImDrawDataSerializer::serializeCharEvent(const CharEvent& event) {
    writeUint32(event.char_code);
}

}  // namespace spatial::debugger