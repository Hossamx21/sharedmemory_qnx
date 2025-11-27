#pragma once
#include "shm_chunk_allocator.hpp"
#include "chunk_queue.hpp"
#include "pulse_notifier.hpp"
#include "region_header.hpp"

class Subscriber {
public:
    Subscriber(ShmChunkAllocator& allocator,
               ChunkQueue& queue,
               PulseNotifier& notifier,
               RegionHeader* hdr)
        : allocator_(allocator),
          queue_(queue),
          notifier_(notifier),
          hdr_(hdr)
    {}

    // Subscriber waits for pulse
    void* receiveBlocking();

    // Non-blocking version
    void* tryReceive();

    void release(void* ptr);

private:
    ShmChunkAllocator& allocator_;
    ChunkQueue& queue_;
    PulseNotifier& notifier_;
    RegionHeader* hdr_;
};