#include "chunk_queue.hpp"

ChunkQueue::ChunkQueue(std::size_t capacity)
    : capacity_(capacity)
{
    buffer_ = new std::size_t[capacity_];
}

ChunkQueue::~ChunkQueue() {
    delete[] buffer_;
}

bool ChunkQueue::push(std::size_t index) {
    auto t = tail_.load(std::memory_order_relaxed);
    auto next = (t + 1) % capacity_;

    if (next == head_.load(std::memory_order_acquire))
        return false; // queue full

    buffer_[t] = index;
    tail_.store(next, std::memory_order_release);
    return true;
}

bool ChunkQueue::pop(std::size_t& outIndex) {
    auto h = head_.load(std::memory_order_relaxed);

    if (h == tail_.load(std::memory_order_acquire))
        return false; // empty

    outIndex = buffer_[h];
    head_.store((h + 1) % capacity_, std::memory_order_release);
    return true;
}

bool ChunkQueue::empty() const noexcept {
    return head_.load() == tail_.load();
}

bool ChunkQueue::full() const noexcept {
    return ((tail_.load() + 1) % capacity_) == head_.load();
}
