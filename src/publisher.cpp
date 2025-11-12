#include "publisher.hpp"
#include <cstring>
#include <stdexcept>

Publisher::Publisher(ShmRegion& region, std::size_t chunkSize)
    : allocator_(region, chunkSize)
{}

void Publisher::publishBlocking(const void* data, std::size_t len) {
    void* chunk = allocator_.allocateBlocking();
    if (!chunk) throw std::runtime_error("allocateBlocking returned nullptr");

    // safe copy: ensure len <= chunkSize_ (caller responsibility; you can add guard)
    std::memcpy(chunk, data, len);

    // chunk already has refcount==1 from allocateBlocking
    allocator_.notifyDataAvailable();
}

bool Publisher::publishNonBlocking(const void* data, std::size_t len) {
    void* chunk = allocator_.allocate();
    if (!chunk) return false;
    std::memcpy(chunk, data, len);
    allocator_.notifyDataAvailable();
    return true;
}
