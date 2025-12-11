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


    void publish(const void* data, std::size_t len);

private:
    void notifyAllSubscribers();

    ShmChunkAllocator& allocator_;
    RegionHeader* hdr_;
    
    // Cache for Connection IDs
    std::vector<int> coids_;
};