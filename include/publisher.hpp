#pragma once
#include "shm_chunk_allocator.hpp"
#include <cstddef>
#include <cstring>
#include <stdexcept>

class Publisher {
public:
    Publisher(ShmRegion& region, std::size_t chunkSize);
    void publish(const void* data, std::size_t len);

private:
    ShmChunkAllocator allocator_;
};
