#pragma once
#include <atomic>
#include <cstdint>

struct alignas(uint64_t) ChunkHeader {
    std::atomic<uint32_t> refCount;
    std::atomic<uint8_t>  inUse;
    uint8_t pad[64 - sizeof(std::atomic<uint32_t>) - sizeof(std::atomic<uint8_t>)];
};
