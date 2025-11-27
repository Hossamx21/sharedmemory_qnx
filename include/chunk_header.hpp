#pragma once
#include <atomic>
#include <cstdint>

struct alignas(8) ChunkHeader {
    std::atomic<uint32_t> refCount;
    std::atomic<uint8_t>  inUse;
    // Padding to ensure 64-byte alignment/size if needed, 
    // or just let the compiler handle natural alignment.
    // For this specific allocator logic, explicit padding isn't strictly fatal 
    // unless you rely on exact math, but let's keep it simple:
    uint8_t pad[3]; 
};