#pragma once
#include "shm_chunk_allocator.hpp"
#include "chunk_queue.hpp"
#include "pulse_notifier.hpp"
#include "region_header.hpp"

class Subscriber {
public:
    Subscriber(ShmChunkAllocator& allocator,
               PulseNotifier& notifier,
               RegionHeader* hdr);
        


    // Subscriber waits for pulse
    void* receiveBlocking();

    // Non-blocking version
    void* tryReceive();

    void release(void* ptr);

    void registerToHeader(); //initialize all receivers

    void* receiveLatest();


private:
    ShmChunkAllocator& allocator_;
    PulseNotifier& notifier_;
    RegionHeader* hdr_;
};