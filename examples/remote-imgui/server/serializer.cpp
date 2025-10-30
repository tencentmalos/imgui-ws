#include "serializer.h"

#include <algorithm>
#include <cstring>
#include <iostream>

#include "../remote_debugger/net_packet_encoder.hpp"
#include "net_imgui_define.hpp"

namespace spatial::debugger {

// Magic and version constants
constexpr uint32_t IMGUI_FRAME_MAGIC = 0x494D4752;  // "IMGR"
constexpr uint32_t IMGUI_PROTOCOL_VERSION = 1;

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
  writeUint32(IMGUI_FRAME_MAGIC);
  writeUint32(IMGUI_PROTOCOL_VERSION);
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

  writeUint32(list_header.vtx_buffer_size);
  writeUint32(list_header.idx_buffer_size);
  writeUint32(list_header.cmd_count);

  // Write vertex buffer
  if (draw_list->VtxBuffer.Size > 0) {
    writeBytes(reinterpret_cast<const uint8_t*>(draw_list->VtxBuffer.Data),
               draw_list->VtxBuffer.Size * sizeof(ImDrawVert));
  }

  // Write index buffer
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
    serialized_cmd.texture_id = reinterpret_cast<uintptr_t>(cmd.GetTexID());

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

    writeUint32(serialized_cmd.idx_count);
    writeUint32(serialized_cmd.clip_rect[0]);
    writeUint32(serialized_cmd.clip_rect[1]);
    writeUint32(serialized_cmd.clip_rect[2]);
    writeUint32(serialized_cmd.clip_rect[3]);
    writeUint32(serialized_cmd.texture_id);
    writeUint32(serialized_cmd.user_callback);
    writeUint32(serialized_cmd.user_callback_data_size);

    // Write user callback data if present
    if (serialized_cmd.user_callback_data_size > 0 &&
        cmd.UserCallbackData != nullptr) {
      const uint8_t* callback_data =
          reinterpret_cast<const uint8_t*>(cmd.UserCallbackData);
      writeBytes(callback_data, serialized_cmd.user_callback_data_size);
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

std::vector<uint8_t> ImDrawDataSerializer::createNetworkPacket(
    ServiceType service_type, uint32_t service_cmd, const uint8_t* data,
    size_t data_size, uint32_t packet_id) {
  // Create header
  NetworkHeader header = NetworkProtocol::createHeader(
      service_type, service_cmd, data_size, packet_id);

  // Create packet
  std::vector<uint8_t> packet;
  packet.resize(HEADER_SIZE + data_size);

  // Copy header
  memcpy(packet.data(), &header, sizeof(header));

  // Copy data
  if (data && data_size > 0) {
    memcpy(packet.data() + sizeof(header), data, data_size);
  }

  return packet;
}

}  // namespace spatial::debugger
