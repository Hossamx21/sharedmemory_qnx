#include "chunk_queue.hpp"
#include "shm_layout.hpp" 

ChunkQueue::ChunkQueue(QueueControlBlock* cb, std::size_t* buffer, std::size_t capacity)
    : capacity_(capacity), cb_(cb), buffer_(buffer) 
{
}

bool ChunkQueue::push(std::size_t index) {
    auto t = cb_->tail.load(std::memory_order_relaxed);
    auto next = (t + 1) % capacity_;

    if (next == cb_->head.load(std::memory_order_acquire))
        return false; 

    buffer_[t] = index;
    cb_->tail.store(next, std::memory_order_release);
    return true;
}

bool ChunkQueue::pop(std::size_t& outIndex) {
    auto h = cb_->head.load(std::memory_order_relaxed);

    if (h == cb_->tail.load(std::memory_order_acquire))
        return false; 

    outIndex = buffer_[h];
    cb_->head.store((h + 1) % capacity_, std::memory_order_release);
    return true;
}