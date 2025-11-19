#include "subscriber.hpp"

void* Subscriber::receiveBlocking() {
    notifier_.createReceiver(hdr_);  // writes pid/chid to shared memory

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
