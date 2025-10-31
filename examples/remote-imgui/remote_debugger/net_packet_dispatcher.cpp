#include "net_packet_dispatcher.hpp"

namespace spatial::debugger {

NetPacketDispatcher::NetPacketDispatcher() {}

void NetPacketDispatcher::AppendIncommingData(const uint8_t* data, size_t size) {
    incoming_data_buffer_.insert(incoming_data_buffer_.end(), data, data + size);
    while (HandleMessageByStatus()) {
        ;   //Just loop here when has contents in incomming buffer
    }
}

bool NetPacketDispatcher::TryReadHeaderFromRawData(const uint8_t* data, size_t size,
                                                   NetPacketHeader& fillHeader) {
    if (!data || size < kNetPacketHeaderSize) { return false; }

    memcpy(&fillHeader, data, kNetPacketHeaderSize);
    return true;
}

void NetPacketDispatcher::DispatchOnePacket(const NetPacketBuffer& packet) {
    auto& header = packet.GetHeader();

    assert((int) header.service_type < kNetMaxServiceNums && "service type must in range here!");
    auto& handler = service_handler_array_[(int) header.service_type];
    if (handler) [[likely]] {
        handler((NetServiceType) header.service_type, header.service_cmd, packet);
    }

    // Process the packet
    total_packets_received_++;
    total_bytes_received_ += packet.TotalSize();
}

bool NetPacketDispatcher::HandleMessageByStatus() {
    switch (current_message_status_) {
    case MessageHandleStatus::ReadHead:
    {
        NetPacketHeader header;
        bool readSuc = TryReadHeaderFromRawData(incoming_data_buffer_.data(), incoming_data_buffer_.size(), header);
        if (readSuc) {
            incoming_data_buffer_.erase(incoming_data_buffer_.begin(), incoming_data_buffer_.begin() + kNetPacketHeaderSize);
            current_message_status_ = MessageHandleStatus::ReadBody;
            cached_header_ = header;
            return true;
        } else {
            //Not need continue here
            return false;
        }
    } break;
    case MessageHandleStatus::ReadBody:
    {
        if (incoming_data_buffer_.size() >= cached_header_.data_length) {
            cached_packet_.Initialize(cached_header_, incoming_data_buffer_.data(), cached_header_.data_length);
            incoming_data_buffer_.erase(incoming_data_buffer_.begin(), incoming_data_buffer_.begin() + cached_header_.data_length);
            current_message_status_ = MessageHandleStatus::Dispatcher;
            return true;
        } else {
            return false;
        }
    } break;
    case MessageHandleStatus::Dispatcher:
    {
        DispatchOnePacket(cached_packet_);
        cached_packet_.Clear();
        current_message_status_ = MessageHandleStatus::ReadHead;
        return true;
    } break;
    }

    return false;
}

}// namespace spatial::debugger
