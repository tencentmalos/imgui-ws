#pragma once

#include "network_protocol.h"
#include "deserializer.h"
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

// Network data processor for handling packet fragmentation and reassembly
class NetworkProcessor {
public:
    // Packet handler callback
    using PacketHandler = std::function<void(ServiceType service_type, uint32_t service_cmd, const std::vector<uint8_t>& data)>;

    NetworkProcessor();
    ~NetworkProcessor() = default;

    // Process incoming data
    void processIncomingData(const uint8_t* data, size_t size);

    // Set packet handler callback
    void setPacketHandler(PacketHandler handler) { packet_handler_ = handler; }

    // Clear all buffers
    void clear();

    // Get statistics
    size_t getActiveBuffersCount() const { return packet_buffers_.size(); }

private:
    // Handle complete packet
    void handleCompletePacket(const NetworkHeader& header, const uint8_t* data, size_t size);

    // Cleanup old packet buffers
    void cleanupOldBuffers();

    // Packet reassembly buffers
    std::unordered_map<uint32_t, PacketBuffer> packet_buffers_;

    // Temporary buffer for incomplete headers
    std::vector<uint8_t> header_buffer_;

    // Packet handler callback
    PacketHandler packet_handler_;

    // Statistics
    uint64_t total_packets_received_ = 0;
    uint64_t total_bytes_received_ = 0;
    uint64_t complete_messages_processed_ = 0;
};