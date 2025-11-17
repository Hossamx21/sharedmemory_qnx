#pragma once
#include <cstdint>

struct RegionHeader {
    uint32_t magic;
    uint32_t capacity;
    uint32_t chunkSize;
    uint32_t headerSize;       // bytes before payload starts
    uint32_t payloadOffset;    // = headerSize
};
