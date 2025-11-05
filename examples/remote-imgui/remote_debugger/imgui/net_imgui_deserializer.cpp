// Include ImGui headers first to resolve type definitions
#include "imgui.h"

#include "net_imgui_deserializer.hpp"

#include <cstring>
#include <iostream>

namespace spatial::debugger {

ImDrawDataDeserializer::ImDrawDataDeserializer() {
    current_frame_ = std::make_unique<FrameData>();
    current_font_texture_ = std::make_unique<FontTextureData>();
}

ImDrawDataDeserializer::~ImDrawDataDeserializer() { Clear(); }

bool ImDrawDataDeserializer::DeserializePacket(const NetPacketBuffer& packet) {
    auto& temp_data = packet.GetContents();

    if (temp_data.empty()) [[unlikely]] {
        std::cerr << "Invalid packet data" << std::endl;
        return false;
    }

    auto size = temp_data.size();

    // Parse frame header
    size_t data_offset = 0;
    if (!ParseFrameHeader(temp_data, size, data_offset)) {
        std::cerr << "Failed to parse frame header" << std::endl;
        return false;
    }

    // Parse draw lists
    if (!ParseDrawLists(temp_data, size, data_offset)) {
        std::cerr << "Failed to parse draw lists" << std::endl;
        return false;
    }

    std::cout << "Successfully deserialized ImGui frame: " << current_frame_->header.cmd_lists_count
              << " draw lists, " << current_frame_->cmd_lists.size() << " cmdlist data entries"
              << std::endl;

    return ValidateData();
}

bool ImDrawDataDeserializer::ParseFrameHeader(const std::vector<uint8_t>& data, size_t size,
                                              size_t& offset) {
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
    memcpy(&current_frame_->header.cmd_lists_count, &data[offset], sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // Validate magic number and version
    if (current_frame_->header.magic != IMGUI_FRAME_MAGIC) {
        std::cerr << "Invalid magic number: 0x" << std::hex << current_frame_->header.magic
                  << std::endl;
        return false;
    }

    if (current_frame_->header.version != IMGUI_PROTOCOL_VERSION) {
        std::cerr << "Unsupported protocol version: " << current_frame_->header.version
                  << std::endl;
        return false;
    }

    // Read display information
    memcpy(&current_frame_->header.display_pos[0], &data[offset], sizeof(float) * 6);
    offset += sizeof(float) * 6;

    return true;
}

bool ImDrawDataDeserializer::ParseDrawLists(const std::vector<uint8_t>& data, size_t size,
                                            size_t& offset) {
    // Clear previous cmdlist-based data
    current_frame_->cmd_lists.clear();

    // Pre-allocate space for cmdlists
    current_frame_->cmd_lists.reserve(current_frame_->header.cmd_lists_count);

    // Parse each draw list
    for (uint32_t i = 0; i < current_frame_->header.cmd_lists_count; i++) {
        if (offset + sizeof(DrawListHeader) > size) {
            std::cerr << "Not enough data for draw list header" << std::endl;
            return false;
        }

        // Read draw list header
        DrawListHeader list_header;
        memcpy(&list_header, &data[offset], sizeof(DrawListHeader));
        offset += sizeof(DrawListHeader);

        // Create new cmdlist data
        CmdListData cmdlist_data;

        // Read vertex buffer
        if (list_header.vtx_buffer_size > 0) {
            if (offset + list_header.vtx_buffer_size > size) {
                std::cerr << "Not enough data for vertex buffer" << std::endl;
                return false;
            }

            size_t vertex_count = list_header.vtx_buffer_size / sizeof(ImDrawVert);
            const ImDrawVert* vertices = reinterpret_cast<const ImDrawVert*>(&data[offset]);

            cmdlist_data.vertex_buffer.resize(vertex_count);
            memcpy(cmdlist_data.vertex_buffer.data(), vertices, list_header.vtx_buffer_size);
            offset += list_header.vtx_buffer_size;
        }

        // Read index buffer
        if (list_header.idx_buffer_size > 0) {
            if (offset + list_header.idx_buffer_size > size) {
                std::cerr << "Not enough data for index buffer" << std::endl;
                return false;
            }

            size_t index_count = list_header.idx_buffer_size / sizeof(ImDrawIdx);
            const ImDrawIdx* indices = reinterpret_cast<const ImDrawIdx*>(&data[offset]);

            cmdlist_data.index_buffer.resize(index_count);
            memcpy(cmdlist_data.index_buffer.data(), indices, list_header.idx_buffer_size);
            offset += list_header.idx_buffer_size;
        }

        // Read draw commands
        cmdlist_data.draw_commands.reserve(list_header.cmd_count);
        for (uint32_t j = 0; j < list_header.cmd_count; j++) {
            // Check if we have enough data for the fixed part of DrawCmd
            size_t fixed_cmd_size = offsetof(DrawCmd, user_callback_data) - offsetof(DrawCmd, idx_count);
            if (offset + fixed_cmd_size > size) {
                std::cerr << "Not enough data for draw command" << std::endl;
                return false;
            }

            DrawCmd cmd;

            // Read the fixed part of DrawCmd as a single block
            memcpy(&cmd.idx_count, &data[offset], fixed_cmd_size);
            offset += fixed_cmd_size;

            // Read user callback data if present
            if (cmd.user_callback_data_size > 0) {
                if (offset + cmd.user_callback_data_size > size) {
                    std::cerr << "Not enough data for user callback data" << std::endl;
                    return false;
                }

                cmd.user_callback_data.resize(cmd.user_callback_data_size);
                memcpy(cmd.user_callback_data.data(), &data[offset], cmd.user_callback_data_size);
                offset += cmd.user_callback_data_size;
            }

            cmdlist_data.draw_commands.push_back(cmd);
        }

        // Add cmdlist to frame data
        current_frame_->cmd_lists.push_back(std::move(cmdlist_data));
    }

    return true;
}

const FrameData* ImDrawDataDeserializer::GetFrameData() const {
    return current_frame_ ? current_frame_.get() : nullptr;
}

bool ImDrawDataDeserializer::DeserializeFontTexture(const NetPacketBuffer& packet) {
    auto& temp_data = packet.GetContents();

    if (temp_data.empty()) [[unlikely]] {
        std::cerr << "Invalid font texture packet data" << std::endl;
        return false;
    }

    size_t size = temp_data.size();
    size_t offset = 0;

    // Check minimum size for header
    if (size < sizeof(FontTextureHeader)) {
        std::cerr << "Font texture packet too small for header" << std::endl;
        return false;
    }

    // Read font texture header
    memcpy(&current_font_texture_->header, &temp_data[offset], sizeof(FontTextureHeader));
    offset += sizeof(FontTextureHeader);

    // Validate header data
    if (current_font_texture_->header.width == 0 ||
        current_font_texture_->header.height == 0 ||
        current_font_texture_->header.data_size == 0) {
        std::cerr << "Invalid font texture dimensions" << std::endl;
        return false;
    }

    // Check if we have enough data for pixel data
    if (size < sizeof(FontTextureHeader) + current_font_texture_->header.data_size) {
        std::cerr << "Font texture packet incomplete" << std::endl;
        return false;
    }

    // Read pixel data
    current_font_texture_->pixel_data.resize(current_font_texture_->header.data_size);
    memcpy(current_font_texture_->pixel_data.data(),
           &temp_data[offset],
           current_font_texture_->header.data_size);

    // Debug: Check first few pixels to verify data integrity
    std::cout << "Deserialized font texture: "
              << current_font_texture_->header.width << "x" << current_font_texture_->header.height
              << ", format: " << current_font_texture_->header.format
              << ", data size: " << current_font_texture_->header.data_size << " bytes" << std::endl;

    if (!current_font_texture_->pixel_data.empty()) {
        std::cout << "First few deserialized pixel values: ";
        for (int i = 0; i < std::min(16, (int)current_font_texture_->pixel_data.size()); i++) {
            printf("%02X ", current_font_texture_->pixel_data[i]);
        }
        std::cout << std::endl;
    }

    return true;
}

const FontTextureData* ImDrawDataDeserializer::GetFontTextureData() const {
    return current_font_texture_ ? current_font_texture_.get() : nullptr;
}

void ImDrawDataDeserializer::Clear() {
    current_frame_.reset();
    current_frame_ = std::make_unique<FrameData>();
    current_font_texture_.reset();
    current_font_texture_ = std::make_unique<FontTextureData>();
}

bool ImDrawDataDeserializer::HasValidFrame() const {
    if (!current_frame_) return false;

    return (current_frame_->header.magic == IMGUI_FRAME_MAGIC &&
            current_frame_->header.version == IMGUI_PROTOCOL_VERSION &&
            current_frame_->header.cmd_lists_count > 0);
}

bool ImDrawDataDeserializer::ValidateData() const {
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

    // Validate cmdlist count matches actual data
    if (current_frame_->cmd_lists.size() != current_frame_->header.cmd_lists_count) {
        std::cerr << "CmdList count mismatch: expected " << current_frame_->header.cmd_lists_count
                  << ", got " << current_frame_->cmd_lists.size() << std::endl;
        return false;
    }

    // Validate each cmdlist has proper data
    for (size_t i = 0; i < current_frame_->cmd_lists.size(); i++) {
        const CmdListData& cmdlist = current_frame_->cmd_lists[i];

        // Validate that vertex and index buffers match command expectations
        // This is basic validation - more thorough checks could be added if needed
        if (!cmdlist.vertex_buffer.empty() && cmdlist.draw_commands.empty()) {
            std::cerr << "CmdList " << i << " has vertex data but no draw commands" << std::endl;
            return false;
        }

        if (cmdlist.index_buffer.empty() && !cmdlist.draw_commands.empty()) {
            std::cerr << "CmdList " << i << " has draw commands but no index data" << std::endl;
            return false;
        }
    }

    return true;
}

}// namespace spatial::debugger