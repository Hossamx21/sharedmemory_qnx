#pragma once

#include "shm_chunk_allocator.hpp"
#include "chunk_queue.hpp"
#include "pulse_notifier.hpp"
#include <cstring>
class Publisher {
public:
    Publisher(ShmChunkAllocator& allocator,
              ChunkQueue& queue,
              PulseNotifier& notifier,
              RegionHeader* hdr)
        : allocator_(allocator),
          queue_(queue),
          notifier_(notifier),
          hdr_(hdr)
    {}

    void publish(const void* data, std::size_t len);

private:
    ShmChunkAllocator& allocator_;
    ChunkQueue& queue_;
    PulseNotifier& notifier_;
    RegionHeader* hdr_;
};
