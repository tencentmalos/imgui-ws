#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

// Network protocol header for handling packet fragmentation and reassembly
#pragma pack(push, 1)
struct NetworkHeader {
    uint32_t magic;           // Magic number for validation: 0x4E455454 ("NETT")
    uint32_t version;         // Protocol version
    uint32_t service_type;    // Service type identifier
    uint32_t service_cmd;     // Command identifier
    uint32_t data_length;     // Length of the payload data
    uint32_t packet_id;       // Packet identifier for ordering
    uint32_t total_packets;   // Total packets in this message
    uint32_t current_packet;  // Current packet number (0-based)
    uint32_t checksum;        // Simple checksum for validation
};
#pragma pack(pop)

// Service types
enum class ServiceType : uint32_t {
    IMGUI_DATA = 0x1,        // ImGui rendering data
    CONTROL = 0x2,           // Control commands
    HEARTBEAT = 0x3,         // Heartbeat/ping
    CONFIG = 0x4,            // Configuration
    CUSTOM = 0x8000          // Custom services start here
};

// ImGui service commands
enum class ImGuiCommand : uint32_t {
    FRAME_DATA = 0x1,        // Complete frame data
    FRAME_PART = 0x2,        // Partial frame data
    REQUEST_CONNECT = 0x3,   // Client wants to connect
    DISCONNECT = 0x4,        // Client disconnect notification
    PING = 0x5,              // Ping request
    PONG = 0x6               // Ping response
};

// Control service commands
enum class ControlCommand : uint32_t {
    SHUTDOWN = 0x1,          // Server shutdown
    RESTART = 0x2,           // Server restart
    STATUS = 0x3,            // Status request/response
    CONFIG_UPDATE = 0x4      // Configuration update
};

// Constants
constexpr uint32_t NETWORK_MAGIC = 0x4E455454; // "NETT"
constexpr uint32_t PROTOCOL_VERSION = 1;
constexpr uint32_t MAX_PACKET_SIZE = 64 * 1024; // 64KB max packet size
constexpr uint32_t HEADER_SIZE = sizeof(NetworkHeader);

// Protocol utilities
class NetworkProtocol {
public:
    // Calculate simple checksum
    static uint32_t calculateChecksum(const uint8_t* data, size_t size) {
        uint32_t checksum = 0;
        for (size_t i = 0; i < size; i++) {
            checksum = ((checksum << 1) | (checksum >> 31)) ^ data[i];
        }
        return checksum;
    }

    // Create header for a single packet message
    static NetworkHeader createHeader(
        ServiceType service_type,
        uint32_t service_cmd,
        uint32_t data_length,
        uint32_t packet_id = 0) {

        NetworkHeader header = {};
        header.magic = NETWORK_MAGIC;
        header.version = PROTOCOL_VERSION;
        header.service_type = static_cast<uint32_t>(service_type);
        header.service_cmd = service_cmd;
        header.data_length = data_length;
        header.packet_id = packet_id;
        header.total_packets = 1;
        header.current_packet = 0;

        // Calculate checksum (excluding checksum field itself)
        header.checksum = calculateChecksum(
            reinterpret_cast<const uint8_t*>(&header),
            sizeof(header) - sizeof(header.checksum)
        );

        return header;
    }

    // Create header for multi-packet message
    static NetworkHeader createMultiPacketHeader(
        ServiceType service_type,
        uint32_t service_cmd,
        uint32_t total_data_length,
        uint32_t packet_id,
        uint32_t total_packets,
        uint32_t current_packet,
        uint32_t packet_data_length) {

        NetworkHeader header = {};
        header.magic = NETWORK_MAGIC;
        header.version = PROTOCOL_VERSION;
        header.service_type = static_cast<uint32_t>(service_type);
        header.service_cmd = service_cmd;
        header.data_length = packet_data_length;
        header.packet_id = packet_id;
        header.total_packets = total_packets;
        header.current_packet = current_packet;

        // Calculate checksum
        header.checksum = calculateChecksum(
            reinterpret_cast<const uint8_t*>(&header),
            sizeof(header) - sizeof(header.checksum)
        );

        return header;
    }

    // Validate header
    static bool validateHeader(const NetworkHeader& header) {
        if (header.magic != NETWORK_MAGIC) {
            return false;
        }
        if (header.version != PROTOCOL_VERSION) {
            return false;
        }
        if (header.data_length > MAX_PACKET_SIZE) {
            return false;
        }

        // Verify checksum
        uint32_t calculated_checksum = calculateChecksum(
            reinterpret_cast<const uint8_t*>(&header),
            sizeof(header) - sizeof(header.checksum)
        );

        return calculated_checksum == header.checksum;
    }

    // Get service type name for debugging
    static const char* getServiceTypeName(ServiceType type) {
        switch (type) {
            case ServiceType::IMGUI_DATA: return "IMGUI_DATA";
            case ServiceType::CONTROL: return "CONTROL";
            case ServiceType::HEARTBEAT: return "HEARTBEAT";
            case ServiceType::CONFIG: return "CONFIG";
            default: return "UNKNOWN";
        }
    }

    // Get command name for debugging
    static const char* getCommandName(ServiceType type, uint32_t cmd) {
        if (type == ServiceType::IMGUI_DATA) {
            switch (static_cast<ImGuiCommand>(cmd)) {
                case ImGuiCommand::FRAME_DATA: return "FRAME_DATA";
                case ImGuiCommand::FRAME_PART: return "FRAME_PART";
                case ImGuiCommand::REQUEST_CONNECT: return "REQUEST_CONNECT";
                case ImGuiCommand::DISCONNECT: return "DISCONNECT";
                case ImGuiCommand::PING: return "PING";
                case ImGuiCommand::PONG: return "PONG";
                default: return "UNKNOWN_IMGUI_CMD";
            }
        } else if (type == ServiceType::CONTROL) {
            switch (static_cast<ControlCommand>(cmd)) {
                case ControlCommand::SHUTDOWN: return "SHUTDOWN";
                case ControlCommand::RESTART: return "RESTART";
                case ControlCommand::STATUS: return "STATUS";
                case ControlCommand::CONFIG_UPDATE: return "CONFIG_UPDATE";
                default: return "UNKNOWN_CONTROL_CMD";
            }
        }
        return "UNKNOWN_CMD";
    }
};

// Packet reassembly buffer
class PacketBuffer {
public:
    PacketBuffer() : total_packets_(0), received_packets_(0), packet_id_(0) {}

    // Initialize for a new multi-packet message
    void initialize(uint32_t packet_id, uint32_t total_packets) {
        packet_id_ = packet_id;
        total_packets_ = total_packets;
        received_packets_ = 0;
        packets_.clear();
        packets_.resize(total_packets);
        packet_received_.clear();
        packet_received_.resize(total_packets, false);
    }

    // Add a packet to the buffer
    bool addPacket(uint32_t packet_num, const uint8_t* data, size_t size) {
        if (packet_num >= total_packets_) {
            return false;
        }

        if (packet_received_[packet_num]) {
            return true; // Already received this packet
        }

        packets_[packet_num].assign(data, data + size);
        packet_received_[packet_num] = true;
        received_packets_++;

        return true;
    }

    // Check if all packets have been received
    bool isComplete() const {
        return received_packets_ == total_packets_ && total_packets_ > 0;
    }

    // Get the reassembled data
    std::vector<uint8_t> getReassembledData() const {
        std::vector<uint8_t> result;
        for (const auto& packet : packets_) {
            result.insert(result.end(), packet.begin(), packet.end());
        }
        return result;
    }

    // Get packet info
    uint32_t getPacketId() const { return packet_id_; }
    uint32_t getTotalPackets() const { return total_packets_; }
    uint32_t getReceivedPackets() const { return received_packets_; }

    // Clear buffer
    void clear() {
        packets_.clear();
        packet_received_.clear();
        total_packets_ = 0;
        received_packets_ = 0;
        packet_id_ = 0;
    }

private:
    std::vector<std::vector<uint8_t>> packets_;
    std::vector<bool> packet_received_;
    uint32_t total_packets_;
    uint32_t received_packets_;
    uint32_t packet_id_;
};