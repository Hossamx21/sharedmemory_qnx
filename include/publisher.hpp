#pragma once
#include "shm_chunk_allocator.hpp"
#include <cstddef>

class Publisher {
public:
    Publisher(ShmRegion& region, std::size_t chunkSize);
    // Blocking publish: wait for a free chunk, write data, notify subscribers.
    void publishBlocking(const void* data, std::size_t len);

    // Non-blocking publish: returns false if no chunk available.
    bool publishNonBlocking(const void* data, std::size_t len);

private:
    ShmChunkAllocator allocator_;
};
