#pragma once

#include "shm_chunk_allocator.hpp"
#include "chunk_queue.hpp"
#include "notifier.hpp"

class Subscriber {
public:
    Subscriber(ShmChunkAllocator& allocator,
               ChunkQueue& queue,
               Notifier& notifier)
        : allocator_(allocator),
          queue_(queue),
          notifier_(notifier)
    {}

    // blocks until a message arrives
    void* receiveBlocking();

    // non-blocking version
    void* tryReceive();

    void release(void* ptr);

private:
    ShmChunkAllocator& allocator_;
    ChunkQueue& queue_;
    Notifier& notifier_;
};
