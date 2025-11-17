#include "publisher.hpp"
#include <cstring>

void Publisher::publish(const void* data, std::size_t len) {
    void* ptr = allocator_.allocate();
    if (!ptr)
        return; // queue is full OR no memory. Later: add blocking allocate.

    std::memcpy(ptr, data, len);

    auto idx = allocator_.indexFromPtr(ptr);

    bool pushed = queue_.push(idx);
    if (pushed)
        notifier_.notify();
}
