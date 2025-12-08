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
    // 1. Snapshot the current Head
    // We read where the head is RIGHT NOW.
    auto h = cb_->head.load(std::memory_order_relaxed);

    while (true) {
        // 2. Check Empty (Standard logic)
        auto t = cb_->tail.load(std::memory_order_acquire);
        if (h == t) return false; // Queue is empty, nothing to pop.

        std::size_t val = buffer_[h];

        // 3. Calculate where Head SHOULD go
        auto next = (h + 1) % capacity_;

        // 4. The Atomic Magic (CAS)
        // compare_exchange_strong takes 3 arguments implicitly: (Variable, Expected, Desired)
        // It asks the CPU: "Is cb_->head still equal to 'h'?"
        //    YES -> Set cb_->head = next. Return true. (We won the race!)
        //    NO  -> Update 'h' to the new real value. Return false. (We lost, try again).
        if (cb_->head.compare_exchange_strong(h, next, 
                                              std::memory_order_release, 
                                              std::memory_order_relaxed)) {
            // If we are here, we successfully moved the pointer.
            // The item at buffer_[h] is now ours.
            outIndex = val;
            return true;
        }
        // If we failed, the loop restarts automatically using the updated 'h'.
    }
}


/*bool ChunkQueue::pop(std::size_t& outIndex) {
    auto h = cb_->head.load(std::memory_order_relaxed);

    if (h == cb_->tail.load(std::memory_order_acquire)) //Check if empty
        return false; 

    outIndex = buffer_[h]; //Read index
    cb_->head.store((h + 1) % capacity_, std::memory_order_release);
    return true;
}*/