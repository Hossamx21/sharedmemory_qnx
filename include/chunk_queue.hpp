/*#pragma once
#include "shm_layout.hpp" 
#include <cstddef>

class ChunkQueue {
public:
    // This constructor matches your .cpp file now
    ChunkQueue(QueueControlBlock* cb, std::size_t* buffer, std::size_t capacity);
    ~ChunkQueue() = default;

    bool push(std::size_t index);
    bool pop(std::size_t& outIndex);

private:
    std::size_t capacity_;
    
    // These pointers reference the Shared Memory (via ShmLayout)
    QueueControlBlock* cb_; 
    std::size_t* buffer_;   
};*/