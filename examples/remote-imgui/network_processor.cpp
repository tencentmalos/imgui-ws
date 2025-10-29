#include "network_processor.h"
#include <iostream>
#include <cstring>

NetworkProcessor::NetworkProcessor() {
    header_buffer_.reserve(HEADER_SIZE);
}

void NetworkProcessor::processIncomingData(const uint8_t* data, size_t size) {
    if (!data || size == 0) {
        return;
    }

    total_bytes_received_ += size;

    // Add incoming data to header buffer
    header_buffer_.insert(header_buffer_.end(), data, data + size);

    // Process all complete packets in the buffer
    size_t offset = 0;
    while (offset + HEADER_SIZE <= header_buffer_.size()) {
        // Try to read header
        NetworkHeader header;
        memcpy(&header, header_buffer_.data() + offset, sizeof(header));

        // Validate header
        if (!NetworkProtocol::validateHeader(header)) {
            std::cerr << "Invalid packet header received, skipping data" << std::endl;
            // Clear buffer and start fresh
            header_buffer_.clear();
            return;
        }

        // Check if we have the complete packet
        size_t complete_packet_size = HEADER_SIZE + header.data_length;
        if (offset + complete_packet_size > header_buffer_.size()) {
            // Not enough data for complete packet, wait for more
            break;
        }

        // Extract packet data
        const uint8_t* packet_data = header_buffer_.data() + offset + HEADER_SIZE;

        // Process the packet
        total_packets_received_++;
        handleCompletePacket(header, packet_data, header.data_length);

        // Move to next packet
        offset += complete_packet_size;
    }

    // Remove processed data from buffer
    if (offset > 0) {
        header_buffer_.erase(header_buffer_.begin(), header_buffer_.begin() + offset);
    }

    // Cleanup old buffers periodically
    if (total_packets_received_ % 100 == 0) {
        cleanupOldBuffers();
    }
}

void NetworkProcessor::handleCompletePacket(const NetworkHeader& header, const uint8_t* data, size_t size) {
    // Handle single packet messages
    if (header.total_packets == 1) {
        // Single packet message, process directly
        std::vector<uint8_t> packet_data(data, data + size);

        if (packet_handler_) {
            packet_handler_(
                static_cast<ServiceType>(header.service_type),
                header.service_cmd,
                packet_data
            );
        }

        complete_messages_processed_++;

        std::cout << "Processed complete packet: "
                  << NetworkProtocol::getServiceTypeName(static_cast<ServiceType>(header.service_type))
                  << " " << NetworkProtocol::getCommandName(static_cast<ServiceType>(header.service_type), header.service_cmd)
                  << ", size: " << size << " bytes" << std::endl;
        return;
    }

    // Handle multi-packet messages
    auto it = packet_buffers_.find(header.packet_id);
    if (it == packet_buffers_.end()) {
        // New multi-packet message
        PacketBuffer buffer;
        buffer.initialize(header.packet_id, header.total_packets);

        if (buffer.addPacket(header.current_packet, data, size)) {
            packet_buffers_[header.packet_id] = std::move(buffer);
        }
    } else {
        // Existing multi-packet message
        PacketBuffer& buffer = it->second;

        if (buffer.addPacket(header.current_packet, data, size)) {
            // Check if message is complete
            if (buffer.isComplete()) {
                std::vector<uint8_t> reassembled_data = buffer.getReassembledData();

                if (packet_handler_) {
                    packet_handler_(
                        static_cast<ServiceType>(header.service_type),
                        header.service_cmd,
                        reassembled_data
                    );
                }

                complete_messages_processed_++;

                std::cout << "Processed reassembled message: packet_id=" << header.packet_id
                          << ", total_size=" << reassembled_data.size() << " bytes, packets=" << buffer.getTotalPackets() << std::endl;

                // Remove completed buffer
                packet_buffers_.erase(it);
            }
        }
    }
}

void NetworkProcessor::cleanupOldBuffers() {
    // Simple cleanup: remove buffers that have been around too long
    // In a real implementation, you might want to track timestamps

    if (packet_buffers_.size() > 100) {
        std::cout << "Cleaning up old packet buffers. Current count: " << packet_buffers_.size() << std::endl;

        // Remove the oldest half of buffers
        auto it = packet_buffers_.begin();
        std::advance(it, packet_buffers_.size() / 2);
        packet_buffers_.erase(packet_buffers_.begin(), it);

        std::cout << "Buffer cleanup complete. New count: " << packet_buffers_.size() << std::endl;
    }
}

void NetworkProcessor::clear() {
    packet_buffers_.clear();
    header_buffer_.clear();
    total_packets_received_ = 0;
    total_bytes_received_ = 0;
    complete_messages_processed_ = 0;
}