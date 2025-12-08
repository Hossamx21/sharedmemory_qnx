#pragma once

#include "shm_chunk_allocator.hpp"
#include "chunk_queue.hpp"
#include "region_header.hpp"
#include <vector>
#include <sys/neutrino.h> // For ConnectAttach/Detach

class Publisher {
public:
    // UPDATE: Constructor now only takes Allocator and Header
    // (We removed ChunkQueue& and PulseNotifier&)
    Publisher(ShmChunkAllocator& allocator, RegionHeader* hdr);

    ~Publisher();

    // New method to add multiple queues (AD, Logger, etc.)
    void addQueue(ChunkQueue* queue);

    void publish(const void* data, std::size_t len);

private:
    void notifyAllSubscribers();

    ShmChunkAllocator& allocator_;
    RegionHeader* hdr_;
    
    // UPDATE: We now store a LIST of queues, not just one reference
    std::vector<ChunkQueue*> queues_;

    // Cache for Connection IDs
    std::vector<int> coids_;
};