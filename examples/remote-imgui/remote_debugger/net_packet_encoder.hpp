#pragma once

#include "net_packet_header.hpp"

namespace spatial::debugger {

// Protocol utilities
class NetPacketEncoder {
 public:
  // Create header for a single packet message
  static NetPacketHeader CreaterPacketHeader(NetServiceType service_type,
                                             uint16_t service_cmd,
                                             uint32_t data_length,
                                             uint32_t packet_id = 0) {
    NetPacketHeader header = {};
    header.magic = kNetPacketMagicNumber;
    header.version = kNetProtocolVersion;
    header.service_type = static_cast<uint8_t>(service_type);
    header.service_cmd = service_cmd;
    header.data_length = data_length;
    header.packet_id = packet_id;

    // Calculate checksum (excluding checksum field itself)
    header.checksum =
        CalculateChecksum(reinterpret_cast<const uint8_t*>(&header),
                          sizeof(header) - sizeof(header.checksum));

    return header;
  }

  // Validate header
  static bool ValidatePacketHeader(const NetPacketHeader& header) {
    if (header.magic != kNetPacketMagicNumber) {
      return false;
    }
    if (header.version != kNetProtocolVersion) {
      return false;
    }
    if (header.data_length > kNetPacketMaxSize) {
      return false;
    }

    // Verify checksum
    uint32_t calculated_checksum =
        CalculateChecksum(reinterpret_cast<const uint8_t*>(&header),
                          sizeof(header) - sizeof(header.checksum));

    return calculated_checksum == header.checksum;
  }

 protected:
  // Calculate simple checksum
  static uint32_t CalculateChecksum(const uint8_t* data, size_t size) {
    uint32_t checksum = 0;
    for (size_t i = 0; i < size; i++) {
      checksum = ((checksum << 1) | (checksum >> 31)) ^ data[i];
    }
    return checksum;
  }
};

}  // namespace spatial::debugger
