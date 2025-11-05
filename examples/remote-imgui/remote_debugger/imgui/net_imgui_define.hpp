#pragma once

#include <cstdint>
#include <vector>

// Forward declaration - ImGui headers will be provided by including projects
struct ImDrawVert;
using ImDrawIdx = unsigned short;

namespace spatial::debugger {

// Magic and version constants
constexpr uint32_t IMGUI_FRAME_MAGIC = 0x494D4752;  // "IMGR"
constexpr uint32_t IMGUI_PROTOCOL_VERSION = 1;

// ImGui service commands
enum class ImGuiCommand : uint16_t {
    FrameData = 0x1,         // Complete frame data
    FontTexture = 0x2,       // Font texture data
    TextureUpdate = 0x3,     // Texture update data
};

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
  uint32_t vtx_offset;        // Vertex offset (added to avoid cross-calculation)
  uint32_t idx_offset;        // Index offset (added to avoid cross-calculation)
  uint32_t user_callback;
  uint32_t user_callback_data_size;
  std::vector<uint8_t> user_callback_data;
};

// Font texture data structure
struct FontTextureHeader {
  uint32_t texture_id;        // Unique texture identifier
  uint32_t width;             // Texture width
  uint32_t height;            // Texture height
  uint32_t data_size;         // Pixel data size
  uint32_t format;            // Pixel format (0=Alpha8, 1=RGBA32)
};

struct FontTextureData {
  FontTextureHeader header;
  std::vector<uint8_t> pixel_data;
};

// Per-cmdlist data structure
struct CmdListData {
  std::vector<ImDrawVert> vertex_buffer;
  std::vector<ImDrawIdx> index_buffer;
  std::vector<DrawCmd> draw_commands;
};

// Simplified frame data structure - cmdlist-based organization
struct FrameData {
  FrameHeader header;
  std::vector<CmdListData> cmd_lists;
};


}  // namespace spatial::debugger

