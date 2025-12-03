#include "subscriber.hpp"

Subscriber::Subscriber(ShmChunkAllocator& allocator,
                       ChunkQueue& queue,
                       PulseNotifier& notifier,
                       RegionHeader* hdr)
    : allocator_(allocator),
      queue_(queue),
      notifier_(notifier),
      hdr_(hdr)
{
    // create the channel once per subscriber
    notifier_.createReceiver(hdr_);
}


void* Subscriber::receiveBlocking() {
    // everytime (make it in the channel create area)
    //notifier_.createReceiver(hdr_);  // writes pid/chid to shared memory
    /*calls createReceiver every time receiveBlocking() is called.
      But we only need to create a channel once per subscriber.*/
    notifier_.wait(); // block until publisher sends pulse

    std::size_t idx;
    if (!queue_.pop(idx))
        return nullptr;

    return allocator_.ptrFromIndex(idx);
}

void* Subscriber::tryReceive() {
    std::size_t idx;
    if (!queue_.pop(idx))
        return nullptr;
    return allocator_.ptrFromIndex(idx);
}

void Subscriber::release(void* ptr) {
    allocator_.release(ptr);
}
