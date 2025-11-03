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

  // Initialize from raw packet data (header + payload)
  void InitializeFromData(const uint8_t* data, size_t size) {
    if (size >= kNetPacketHeaderSize) {
      memcpy(&header_, data, kNetPacketHeaderSize);
      const uint8_t* payload = data + kNetPacketHeaderSize;
      size_t payload_size = size - kNetPacketHeaderSize;

      if (payload_size > 0) {
        contents_.assign(payload, payload + payload_size);
      } else {
        contents_.clear();
      }
    } else {
      // Invalid packet size
      memset(&header_, 0, sizeof(header_));
      contents_.clear();
    }
  }

  // Get packet info
  const NetPacketHeader& GetHeader() const { return header_; }

  // Get the reassembled data
  const std::vector<uint8_t>& GetContents() const { return contents_; }

  // Get complete packet data (header + contents)
  std::vector<uint8_t> GetFullPacketData() const {
    std::vector<uint8_t> full_data;
    full_data.resize(TotalSize());

    if (!contents_.empty()) {
      memcpy(full_data.data(), &header_, kNetPacketHeaderSize);
      memcpy(full_data.data() + kNetPacketHeaderSize, contents_.data(), contents_.size());
    }

    return full_data;
  }

  // Get pointer to complete packet data (for network transmission)
  const uint8_t* GetData() const {
    if (contents_.empty()) {
      return nullptr;
    }

    // Note: This is a simplified approach. In a real implementation,
    // you might want to store the data contiguously for efficiency.
    static thread_local std::vector<uint8_t> temp_data;
    temp_data = GetFullPacketData();
    return temp_data.data();
  }

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
