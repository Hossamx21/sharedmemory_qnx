#include "subscriber.hpp"

Subscriber::Subscriber(ShmRegion& region, std::size_t chunkSize)
    : allocator_(region, chunkSize)
{}

void* Subscriber::receiveBlocking() {
    void* ptr = allocator_.waitForData();
    if (ptr) {
        allocator_.retain(ptr);
    }
    return ptr;
}

void* Subscriber::receiveNonBlocking() {
    // Simple scan using allocator internals (we can use wait-free scan via allocator.allocate-like logic)
    // But cleaner: use waitForData with a quick timeout variant; for now, poll with a lock-free check:
    // We'll just scan refCounts without locking (atomic loads are safe)
    for (std::size_t i = 0; i < allocator_.capacity(); ++i) {
        // To keep encapsulation we'd need an accessor; for brevity assume allocator provides a quick check,
        // but here we simply return nullptr to indicate not-implemented in this demo.
    }
    return nullptr;
}

void Subscriber::release(void* ptr) {
    allocator_.release(ptr);
}
