#pragma once
#include <cstdint>

struct RegionHeader {
    uint32_t magic;
    uint32_t capacity;
    uint32_t chunkSize;
    uint32_t headerSize;
    uint32_t payloadOffset;

    // Pulse-based notification fields
    int32_t notify_pid;      
    /*PID of the subscriber process (consumer).
    The process that wants to receive a QNX pulse when new data arrives. */

    int32_t notify_chid;     
    /*Channel ID created by consumer using ChannelCreate().
      Pulses are delivered to this channel.*/
};
