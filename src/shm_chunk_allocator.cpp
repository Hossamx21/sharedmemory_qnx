#include "shm_chunk_allocator.hpp"


ShmChunkAllocator::ShmChunkAllocator(ShmRegion& region, std::size_t chunkSize)
    : region_(region), chunkSize_(chunkSize)
{
    capacity_ = region_.getSize() / chunkSize_;
    chunksBase_ = region_.get();
    refCounts_.resize(capacity_);
    for (auto& ref : refCounts_)
        ref.store(0, std::memory_order_relaxed);
}

void* ShmChunkAllocator::allocate() {
    std::unique_lock lock(mtx_);
    for (std::size_t i = 0; i < capacity_; ++i) {
        uint32_t expected = 0;
        if (refCounts_[i].compare_exchange_strong(expected, 1)) {
            return static_cast<uint8_t*>(chunksBase_) + i * chunkSize_;
        }
    }
    return nullptr; // no free chunk
}

void ShmChunkAllocator::retain(void* ptr) {
    auto index = indexFromPtr(ptr);
    refCounts_[index].fetch_add(1, std::memory_order_relaxed);
}

void ShmChunkAllocator::release(void* ptr) {
    auto index = indexFromPtr(ptr);
    refCounts_[index].fetch_sub(1, std::memory_order_relaxed);
}

std::size_t ShmChunkAllocator::indexFromPtr(void* ptr) const noexcept {
    auto offset = static_cast<uint8_t*>(ptr) - static_cast<uint8_t*>(chunksBase_);
    return offset / chunkSize_;
}
