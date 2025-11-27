#pragma once
#include "region_header.hpp"
#include <atomic>
#include <cstddef>

// This struct lives inside Shared Memory
struct QueueControlBlock {
    std::atomic_size_t head{0};
    std::atomic_size_t tail{0};
};

// The Master Map of the shared memory region
struct ShmLayout {
    RegionHeader header;        
    QueueControlBlock queue;    
    // Data buffer follows immediately after this struct
};