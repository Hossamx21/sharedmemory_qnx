// subscriber.hpp
#pragma once
#include "shm_chunk_allocator.hpp"

class Subscriber {
public:
    Subscriber(ShmRegion& region, std::size_t chunkSize);
    void* receive();  // placeholder for real implementation
    void release(void* ptr);

private:
    ShmChunkAllocator allocator_;
};
