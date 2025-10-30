#pragma once

#include <cstdint>

namespace spatial::debugger {

// ImGui service commands
enum class ImGuiCommand : uint16_t {
    FrameData = 0x1,       // Complete frame data
    TextureData = 0x2,       // Partial frame data
};

}  // namespace spatial::debugger

