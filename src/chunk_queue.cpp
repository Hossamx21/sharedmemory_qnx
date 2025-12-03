#include "chunk_queue.hpp"
#include "shm_layout.hpp" 

ChunkQueue::ChunkQueue(QueueControlBlock* cb, std::size_t* buffer, std::size_t capacity)
    : capacity_(capacity), cb_(cb), buffer_(buffer) 
{
}

bool ChunkQueue::push(std::size_t index) {
    auto t = cb_->tail.load(std::memory_order_relaxed); //Loads tail index atomically.
    auto next = (t + 1) % capacity_; //get the next, Wraps around when reaching end.

    if (next == cb_->head.load(std::memory_order_acquire))  //Check if queue is full
        return false; 

    buffer_[t] = index;  //Stores the chunk ID into shared memory.
    cb_->tail.store(next, std::memory_order_release);
    return true;
    /*memory_order_release ensures:
     Write to buffer happens before updating tail.
    Other processes see a consistent order. */
}

bool ChunkQueue::pop(std::size_t& outIndex) {
    auto h = cb_->head.load(std::memory_order_relaxed);

    if (h == cb_->tail.load(std::memory_order_acquire)) //Check if empty
        return false; 

    outIndex = buffer_[h]; //Read index
    cb_->head.store((h + 1) % capacity_, std::memory_order_release);
    return true;
}