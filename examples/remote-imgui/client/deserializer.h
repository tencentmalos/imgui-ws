#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "../remote_debugger/imgui/net_imgui_define.hpp"
#include "imgui.h"

namespace spatial::debugger {

class ImDrawDataDeserializer {
 public:
  ImDrawDataDeserializer();
  ~ImDrawDataDeserializer();

  // Deserialize binary data
  bool deserializePacket(const uint8_t* data, size_t size);

  // Get deserialized frame data
  const FrameData* getFrameData() const;

  // Clear data
  void clear();

  // Has valid frame data
  bool hasValidFrame() const;

 private:
  std::unique_ptr<FrameData> current_frame_;

  // Parse frame header
  bool parseFrameHeader(const std::vector<uint8_t>& data, size_t size,
                        size_t& offset);

  // Parse draw lists
  bool parseDrawLists(const std::vector<uint8_t>& data, size_t size,
                      size_t& offset);

  // Validate data integrity
  bool validateData() const;
};

}  // namespace spatial::debugger
