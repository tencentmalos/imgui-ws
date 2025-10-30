#pragma once

#include <vector>

#include "net_packet_header.hpp"

namespace spatial::debugger {

// Packet reassembly buffer
class NetPacketBuffer {
 public:
  NetPacketBuffer() { memset(&header_, 0, sizeof(header_)); }

  // Initialize for a new multi-packet message
  void Initialize(const NetPacketHeader& header, const uint8_t* data,
                  size_t size) {
    header_ = header;
    if (size > 0) [[likely]] {
      contents_.assign(data, data + size);
    } else {
      contents_.clear();
    }
  }

  // Get packet info
  const NetPacketHeader& GetHeader() const { return header_; }

  // Get the reassembled data
  const std::vector<uint8_t>& GetContents() const { return contents_; }

  // Clear buffer
  void Clear() {
    memset(&header_, 0, sizeof(header_));
    contents_.clear();
  }

  size_t TotalSize() const { return kNetPacketHeaderSize + contents_.size(); }

 private:
  NetPacketHeader header_;
  std::vector<uint8_t> contents_;
};

}  // namespace spatial::debugger
