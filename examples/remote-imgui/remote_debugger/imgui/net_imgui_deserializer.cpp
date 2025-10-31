// Include ImGui headers first to resolve type definitions
#include "imgui.h"

#include "net_imgui_deserializer.hpp"

#include <cstring>
#include <iostream>

namespace spatial::debugger {

ImDrawDataDeserializer::ImDrawDataDeserializer() {
  current_frame_ = std::make_unique<FrameData>();
}

ImDrawDataDeserializer::~ImDrawDataDeserializer() { clear(); }

bool ImDrawDataDeserializer::deserializePacket(const uint8_t* data,
                                               size_t size) {
  if (!data || size == 0) {
    std::cerr << "Invalid packet data" << std::endl;
    return false;
  }

  // Store data and reset offset
  // Simplified implementation: assume complete data transfer
  std::vector<uint8_t> temp_data(data, data + size);

  // Parse frame header
  size_t data_offset = 0;
  if (!parseFrameHeader(temp_data, size, data_offset)) {
    std::cerr << "Failed to parse frame header" << std::endl;
    return false;
  }

  // Parse draw lists
  if (!parseDrawLists(temp_data, size, data_offset)) {
    std::cerr << "Failed to parse draw lists" << std::endl;
    return false;
  }

  std::cout << "Successfully deserialized ImGui frame: "
            << current_frame_->header.cmd_lists_count << " draw lists, "
            << current_frame_->draw_commands.size() << " draw commands"
            << std::endl;

  return validateData();
}

bool ImDrawDataDeserializer::parseFrameHeader(const std::vector<uint8_t>& data,
                                              size_t size, size_t& offset) {
  if (size < sizeof(FrameHeader)) {
    std::cerr << "Packet too small for frame header" << std::endl;
    return false;
  }

  offset = 0;

  // Read magic number
  memcpy(&current_frame_->header.magic, &data[offset], sizeof(uint32_t));
  offset += sizeof(uint32_t);

  // Read version
  memcpy(&current_frame_->header.version, &data[offset], sizeof(uint32_t));
  offset += sizeof(uint32_t);

  // Read total size
  memcpy(&current_frame_->header.total_size, &data[offset], sizeof(uint32_t));
  offset += sizeof(uint32_t);

  // Read draw list count
  memcpy(&current_frame_->header.cmd_lists_count, &data[offset],
         sizeof(uint32_t));
  offset += sizeof(uint32_t);

  // Validate magic number and version
  if (current_frame_->header.magic != IMGUI_FRAME_MAGIC) {
    std::cerr << "Invalid magic number: 0x" << std::hex
              << current_frame_->header.magic << std::endl;
    return false;
  }

  if (current_frame_->header.version != IMGUI_PROTOCOL_VERSION) {
    std::cerr << "Unsupported protocol version: "
              << current_frame_->header.version << std::endl;
    return false;
  }

  // Read display information
  memcpy(&current_frame_->header.display_pos[0], &data[offset],
         sizeof(float) * 6);
  offset += sizeof(float) * 6;

  return true;
}

bool ImDrawDataDeserializer::parseDrawLists(const std::vector<uint8_t>& data,
                                            size_t size, size_t& offset) {
  // Clear previous data
  current_frame_->vertex_buffers.clear();
  current_frame_->index_buffers.clear();
  current_frame_->draw_commands.clear();
  current_frame_->vertex_offsets.clear();
  current_frame_->index_offsets.clear();
  current_frame_->command_offsets.clear();
  current_frame_->command_counts.clear();

  // Pre-allocate space
  current_frame_->vertex_buffers.reserve(10000);
  current_frame_->index_buffers.reserve(20000);
  current_frame_->draw_commands.reserve(1000);

  // Parse each draw list
  for (uint32_t i = 0; i < current_frame_->header.cmd_lists_count; i++) {
    if (offset + sizeof(DrawListHeader) > size) {
      std::cerr << "Not enough data for draw list header" << std::endl;
      return false;
    }

    // Read draw list header
    DrawListHeader list_header;
    memcpy(&list_header.vtx_buffer_size, &data[offset], sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&list_header.idx_buffer_size, &data[offset], sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&list_header.cmd_count, &data[offset], sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // Record offsets
    current_frame_->vertex_offsets.push_back(
        current_frame_->vertex_buffers.size());
    current_frame_->index_offsets.push_back(
        current_frame_->index_buffers.size());
    current_frame_->command_offsets.push_back(
        current_frame_->draw_commands.size());
    current_frame_->command_counts.push_back(list_header.cmd_count);

    // Read vertex buffer
    if (list_header.vtx_buffer_size > 0) {
      if (offset + list_header.vtx_buffer_size > size) {
        std::cerr << "Not enough data for vertex buffer" << std::endl;
        return false;
      }

      size_t vertex_count = list_header.vtx_buffer_size / sizeof(ImDrawVert);
      const ImDrawVert* vertices =
          reinterpret_cast<const ImDrawVert*>(&data[offset]);

      current_frame_->vertex_buffers.insert(
          current_frame_->vertex_buffers.end(), vertices,
          vertices + vertex_count);
      offset += list_header.vtx_buffer_size;
    }

    // Read index buffer
    if (list_header.idx_buffer_size > 0) {
      if (offset + list_header.idx_buffer_size > size) {
        std::cerr << "Not enough data for index buffer" << std::endl;
        return false;
      }

      size_t index_count = list_header.idx_buffer_size / sizeof(ImDrawIdx);
      const ImDrawIdx* indices =
          reinterpret_cast<const ImDrawIdx*>(&data[offset]);

      current_frame_->index_buffers.insert(current_frame_->index_buffers.end(),
                                           indices, indices + index_count);
      offset += list_header.idx_buffer_size;
    }

    // Read draw commands
    for (uint32_t j = 0; j < list_header.cmd_count; j++) {
      if (offset + sizeof(uint32_t) * 7 >
          size) {  // Updated for new DrawCmd size
        std::cerr << "Not enough data for draw command" << std::endl;
        return false;
      }

      DrawCmd cmd;

      // Read basic command data
      memcpy(&cmd.idx_count, &data[offset], sizeof(uint32_t));
      offset += sizeof(uint32_t);

      memcpy(&cmd.clip_rect[0], &data[offset], sizeof(uint32_t) * 4);
      offset += sizeof(uint32_t) * 4;

      memcpy(&cmd.texture_id, &data[offset], sizeof(uint32_t));
      offset += sizeof(uint32_t);

      // Read UserCallback related data
      memcpy(&cmd.user_callback, &data[offset], sizeof(uint32_t));
      offset += sizeof(uint32_t);

      memcpy(&cmd.user_callback_data_size, &data[offset], sizeof(uint32_t));
      offset += sizeof(uint32_t);

      // Read user callback data if present
      if (cmd.user_callback_data_size > 0) {
        if (offset + cmd.user_callback_data_size > size) {
          std::cerr << "Not enough data for user callback data" << std::endl;
          return false;
        }

        cmd.user_callback_data.resize(cmd.user_callback_data_size);
        memcpy(cmd.user_callback_data.data(), &data[offset],
               cmd.user_callback_data_size);
        offset += cmd.user_callback_data_size;
      }

      current_frame_->draw_commands.push_back(cmd);
    }
  }

  return true;
}

const FrameData* ImDrawDataDeserializer::getFrameData() const {
  return current_frame_ ? current_frame_.get() : nullptr;
}

void ImDrawDataDeserializer::clear() {
  current_frame_.reset();
  current_frame_ = std::make_unique<FrameData>();
}

bool ImDrawDataDeserializer::hasValidFrame() const {
  if (!current_frame_) return false;

  return (current_frame_->header.magic == IMGUI_FRAME_MAGIC &&
          current_frame_->header.version == IMGUI_PROTOCOL_VERSION &&
          current_frame_->header.cmd_lists_count > 0);
}

bool ImDrawDataDeserializer::validateData() const {
  if (!current_frame_) return false;

  // Basic validation
  if (current_frame_->header.magic != IMGUI_FRAME_MAGIC) {
    std::cerr << "Invalid magic number" << std::endl;
    return false;
  }

  if (current_frame_->header.version != IMGUI_PROTOCOL_VERSION) {
    std::cerr << "Invalid protocol version" << std::endl;
    return false;
  }

  // Validate draw list count matches actual data
  if (current_frame_->command_counts.size() !=
      current_frame_->header.cmd_lists_count) {
    std::cerr << "Draw list count mismatch" << std::endl;
    return false;
  }

  // Validate command totals match
  size_t total_commands = 0;
  for (uint32_t count : current_frame_->command_counts) {
    total_commands += count;
  }

  if (total_commands != current_frame_->draw_commands.size()) {
    std::cerr << "Command count mismatch" << std::endl;
    return false;
  }

  return true;
}

}  // namespace spatial::debugger