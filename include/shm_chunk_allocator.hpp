#pragma once
#include "shm_region.hpp"
#include <atomic>
#include <mutex>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <cstring>

class ShmChunkAllocator {
public:
    ShmChunkAllocator(ShmRegion& region, std::size_t chunkSize);

    void* allocate();
    void retain(void* ptr);
    void release(void* ptr);

private:
    std::size_t indexFromPtr(void* ptr) const noexcept;

    ShmRegion& region_;
    std::size_t chunkSize_{};
    std::size_t capacity_{};
    void* chunksBase_{nullptr};
    std::vector<std::atomic<uint32_t>> refCounts_;
    std::mutex mtx_;
};
