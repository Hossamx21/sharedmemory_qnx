#include "subscriber.hpp"

void* Subscriber::receiveBlocking() {
    notifier_.wait();  // block until publisher notifies

    std::size_t idx;
    bool ok = queue_.pop(idx);
    if (!ok)
        return nullptr;  // rare race: notified but no data

    return allocator_.ptrFromIndex(idx);
}

void* Subscriber::tryReceive() {
    std::size_t idx;
    bool ok = queue_.pop(idx);
    if (!ok)
        return nullptr;

    return allocator_.ptrFromIndex(idx);
}

void Subscriber::release(void* ptr) {
    allocator_.release(ptr);
}
