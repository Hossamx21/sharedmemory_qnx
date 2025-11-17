#pragma once
#include <cstddef>
#include <atomic>
#include <cstdint>

class ChunkQueue {
public:
    explicit ChunkQueue(std::size_t capacity);
    ~ChunkQueue();

    bool push(std::size_t index);
    bool pop(std::size_t& outIndex);

    bool empty() const noexcept;
    bool full() const noexcept;

private:
    std::size_t capacity_;
    std::atomic_size_t head_{0};
    std::atomic_size_t tail_{0};
    std::size_t* buffer_;  //in shared memory later
};
