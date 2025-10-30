#include "net_packet_dispatcher.hpp"

namespace spatial::debugger {

NetPacketDispatcher::NetPacketDispatcher() {}

bool NetPacketDispatcher::TryReadHeaderFromRawData(
    const uint8_t* data, size_t size, NetPacketHeader& fillHeader) {
  if (!data || size < kNetPacketHeaderSize) {
    return false;
  }

  memcpy(&fillHeader, data, kNetPacketHeaderSize);
  return true;
}

void NetPacketDispatcher::ProcessIncomingPacket(const NetPacketBuffer& packet) {
  auto& header = packet.GetHeader();

  assert((int)header.service_type < kNetMaxServiceNums && "service type must in range here!");
  auto& handler = service_handler_array_[(int)header.service_type];
  if (handler) [[likely]] {
    handler((NetServiceType)header.service_type, header.service_cmd, packet);
  }

  // Process the packet
  total_packets_received_++;
  total_bytes_received_ += packet.TotalSize();
}

}  // namespace spatial::debugger
