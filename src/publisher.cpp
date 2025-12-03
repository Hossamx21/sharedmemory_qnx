#include "publisher.hpp"

void Publisher::publish(const void* data, std::size_t len) {
    void* ptr = allocator_.allocate();
    if (!ptr)
        return; 

    if (len > allocator_.getChunkSize()) {
    len = allocator_.getChunkSize(); // or return false
    }

    std::memcpy(ptr, data, len);

    std::size_t idx = allocator_.indexFromPtr(ptr);

    if (queue_.push(idx)) {
        notifier_.notify(hdr_);
    } else {
        allocator_.release(ptr); /*The message cannot be delivered now. Free the chunk:*/
    }
}
