#pragma once

#include "net_packet_header.hpp"

#include <cassert>
#include <functional>

#include "net_packet_buffer.hpp"

namespace spatial::debugger {

// Network data processor for handling packet fragmentation and reassembly
class NetPacketDispatcher {
 public:
  // Packet handler callback
  using PacketHandler =
      std::function<void(NetServiceType service_type, uint16_t service_cmd, const NetPacketBuffer& packet)>;

  NetPacketDispatcher();
  ~NetPacketDispatcher() = default;

  bool TryReadHeaderFromRawData(const uint8_t* data, size_t size, NetPacketHeader& fillHeader);

  // Process incoming data
  void ProcessIncomingPacket(const NetPacketBuffer& packet);

  // Set packet handler callback
  void BindServicePacketHandler(NetServiceType service_type, PacketHandler handler) {
    assert((uint32_t)service_type < kNetMaxServiceNums && "service type may not over the max limit here!");
    service_handler_array_[(int)service_type] = handler;
  }
protected:
  // Packet handler callback
  PacketHandler service_handler_array_[kNetMaxServiceNums];

  // Statistics
  uint64_t total_packets_received_ = 0;
  uint64_t total_bytes_received_ = 0;
};

}  // namespace spatial::debugger
