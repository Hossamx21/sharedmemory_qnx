#pragma once
#include <cstdint>

struct RegionHeader {
    uint32_t magic;
    uint32_t capacity;
    uint32_t chunkSize;
    uint32_t headerSize;
    uint32_t payloadOffset;

    // Pulse-based notification fields
    int32_t notify_pid;      // subscriber pid
    int32_t notify_chid;     // subscriber channel id
};
