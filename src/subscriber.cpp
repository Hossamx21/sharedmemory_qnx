#include "subscriber.hpp"

Subscriber::Subscriber(ShmChunkAllocator& allocator,
                       PulseNotifier& notifier,
                       RegionHeader* hdr)
    : allocator_(allocator),
      notifier_(notifier),
      hdr_(hdr)
{
    // create the channel once per subscriber
    //notifier_.createReceiver(hdr_);
    registerToHeader();
}

void Subscriber::registerToHeader() {
    // Ensure we have a channel created
    if (!notifier_.createReceiver(nullptr)) {
        throw std::runtime_error("Failed to create local channel for subscriber");
    }
    int myChid = notifier_.getChid();
    int myPid  = static_cast<int>(getpid());

    for (uint32_t i = 0; i < RegionHeader::MAX_SUBSCRIBERS; ++i) {
        auto &s = hdr_->subscribers[i];
        bool expected = false;
        if (s.active.compare_exchange_strong(expected, true)) {
            s.pid.store(myPid, std::memory_order_release);
            s.chid.store(myChid, std::memory_order_release);
            return;
        }
    }
    // If we get here, no free slot — leave our channel and mark inactive
    // Optionally: close channel to avoid leaks
    notifier_.close();
    throw std::runtime_error("No subscriber slots available");
}


/*void* Subscriber::receiveLatest() {
    uint64_t seq1 = hdr_->latest_seq.load(std::memory_order_acquire);
    uint32_t idx  = hdr_->latest_idx.load(std::memory_order_acquire);

    void* ptr = allocator_.ptrFromIndex(idx);

    uint64_t seq2 = hdr_->latest_seq.load(std::memory_order_acquire);
    if (seq1 != seq2) {
        allocator_.release(ptr);
        return nullptr; // frame changed while reading
    }

    return ptr;
}*/


void* Subscriber::receiveBlocking() {
    // everytime (make it in the channel create area)
    //notifier_.createReceiver(hdr_);  // writes pid/chid to shared memory
    /*calls createReceiver every time receiveBlocking() is called.
      But we only need to create a channel once per subscriber.*/
    notifier_.wait(); // block until publisher sends pulse

     return nullptr;
}

void* Subscriber::tryReceive() {
    return nullptr;
}

void Subscriber::release(void* ptr) {
    allocator_.release(ptr);
}


// Hypothetical cleanup
/*void Subscriber::unregister() {
    // Find my slot and set active = false
    hdr_->subscribers[mySlotIndex].active.store(false);
}*/