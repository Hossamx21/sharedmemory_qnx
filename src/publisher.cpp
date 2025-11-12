// publisher.cpp
#include "publisher.hpp"
#include <cstring>
#include <stdexcept>

Publisher::Publisher(ShmRegion& region, std::size_t chunkSize)
    : allocator_(region, chunkSize) {}

void Publisher::publish(const void* data, std::size_t len) {
    void* chunk = allocator_.allocate();
    if (!chunk)
        throw std::runtime_error("No available chunk in shared memory allocator");

    std::memcpy(chunk, data, len);
}
