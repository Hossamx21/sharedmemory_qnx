#pragma once

#include "shm_chunk_allocator.hpp"
#include "chunk_queue.hpp"
#include "notifier.hpp"
#include <cstddef>

class Publisher {
public:
    Publisher(ShmChunkAllocator& allocator,
              ChunkQueue& queue,
              Notifier& notifier)
        : allocator_(allocator),
          queue_(queue),
          notifier_(notifier)
    {}

    void publish(const void* data, std::size_t len);

private:
    ShmChunkAllocator& allocator_;
    ChunkQueue& queue_;
    Notifier& notifier_;
};
