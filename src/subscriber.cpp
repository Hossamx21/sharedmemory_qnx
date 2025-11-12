#include "subscriber.hpp"

Subscriber::Subscriber(ShmRegion& region, std::size_t chunkSize)
    : allocator_(region, chunkSize) {}

void* Subscriber::receive() {
    // TODO: Implement logic to find next chunk with data
    return nullptr;
}

void Subscriber::release(void* ptr) {
    allocator_.release(ptr);
}
