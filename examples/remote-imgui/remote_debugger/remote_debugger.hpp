#pragma once

// Remote Debugger Library - Network Protocol and ImGui Serialization
// This library provides network communication and ImGui data serialization/deserialization
// for remote debugging applications.

#include "net_packet_header.hpp"
#include "net_packet_buffer.hpp"
#include "net_packet_encoder.hpp"
#include "net_packet_dispatcher.hpp"
#include "imgui/net_imgui_define.hpp"
#include "imgui/net_imgui_serializer.hpp"
#include "imgui/net_imgui_deserializer.hpp"

namespace spatial::debugger {

// Library version information
constexpr uint32_t REMOTE_DEBUGGER_VERSION_MAJOR = 1;
constexpr uint32_t REMOTE_DEBUGGER_VERSION_MINOR = 0;
constexpr uint32_t REMOTE_DEBUGGER_VERSION_PATCH = 0;

// Convenience type aliases for commonly used classes
using PacketDispatcher = NetPacketDispatcher;
using ImGuiSerializer = ImDrawDataSerializer;
using ImGuiDeserializer = ImDrawDataDeserializer;

}  // namespace spatial::debugger