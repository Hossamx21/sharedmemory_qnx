#pragma once
#include "shm_chunk_allocator.hpp"
#include <cstddef>

class Subscriber {
public:
    Subscriber(ShmRegion& region, std::size_t chunkSize);

    // Blocking receive: waits for next available chunk with data, retains it and returns pointer.
    // Caller must call release(ptr) when done.
    void* receiveBlocking();

    // Non-blocking: scan and return first available data chunk or nullptr.
    void* receiveNonBlocking();

    void release(void* ptr);

private:
    ShmChunkAllocator allocator_;
};
