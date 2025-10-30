#pragma once

#include <cstdint>
#include <cstring>

namespace spatial::debugger {

// Network protocol header for handling packet fragmentation and reassembly
#pragma pack(push, 1)
struct NetPacketHeader {
    uint32_t    magic;              // Magic number for validation: 0x4E455454 ("NETT")
    uint16_t    version;            // Protocol version
    uint8_t     request_type;       // 0 for request, 1 for response
    uint8_t     result_code;        // when on response status, 0 is for suc, and other is for errors  
    uint8_t     service_type;       // Service type identifier
    uint16_t    service_cmd;        // Command identifier
    uint32_t    data_length;        // Length of the payload data
    uint32_t    packet_id;          // Packet identifier for ordering
    uint32_t    checksum;           // Simple checksum for validation
};
#pragma pack(pop)

//request type
enum class NetRequestType : uint8_t
{
    Request = 0,
    Response = 1,
};

// Service types
enum class NetServiceType : uint8_t {
  RemoteImgui = 0x1,  // ImGui rendering data
  Custom = 100,    // Custom services start here
};

// Constants
constexpr uint32_t kNetPacketMagicNumber = 0x4E455454;  // "NETT"
constexpr uint32_t kNetProtocolVersion = 1;
constexpr uint32_t kNetPacketMaxSize = 64 * 1024;  // 64KB max packet size
constexpr uint32_t kNetPacketHeaderSize = sizeof(NetPacketHeader);
constexpr uint32_t kNetMaxServiceNums = 256;
constexpr uint32_t kNetMaxCommandNums = 65536;

}  // namespace spatial::debugger

