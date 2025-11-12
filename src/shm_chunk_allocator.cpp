#include "shm_chunk_allocator.hpp"
#include <cstring>      // optional
#include <stdexcept>
#include <system_error>

ShmChunkAllocator::ShmChunkAllocator(ShmRegion& region, std::size_t chunkSize)
    : region_(region), chunkSize_(chunkSize)
{
    if (chunkSize_ == 0) throw std::invalid_argument("chunkSize must be > 0");
    capacity_ = region_.getSize() / chunkSize_;
    chunksBase_ = region_.get();
    refCounts_.resize(capacity_);
    for (auto &a : refCounts_) a.store(0, std::memory_order_relaxed);
}

void* ShmChunkAllocator::allocate() noexcept {
    // Non-blocking, but must protect access to the refCounts vector iteration.
    std::lock_guard<std::mutex> lock(mtx_);
    for (std::size_t i = 0; i < capacity_; ++i) {
        uint32_t expected = 0;
        if (refCounts_[i].compare_exchange_strong(expected, 1, std::memory_order_acq_rel)) {
            return static_cast<uint8_t*>(chunksBase_) + i * chunkSize_;
        }
    }
    return nullptr;
}

void* ShmChunkAllocator::allocateBlocking() {
    std::unique_lock<std::mutex> lock(mtx_);
    // First try immediately
    for (std::size_t i = 0; i < capacity_; ++i) {
        uint32_t expected = 0;
        if (refCounts_[i].compare_exchange_strong(expected, 1, std::memory_order_acq_rel)) {
            return static_cast<uint8_t*>(chunksBase_) + i * chunkSize_;
        }
    }

    // Wait until a chunk is freed (cv_free_ signalled by release())
    cv_free_.wait(lock, [this]() {
        for (std::size_t i = 0; i < capacity_; ++i)
            if (refCounts_[i].load(std::memory_order_acquire) == 0) return true;
        return false;
    });

    // After wake, find & claim the free chunk
    for (std::size_t i = 0; i < capacity_; ++i) {
        uint32_t expected = 0;
        if (refCounts_[i].compare_exchange_strong(expected, 1, std::memory_order_acq_rel)) {
            return static_cast<uint8_t*>(chunksBase_) + i * chunkSize_;
        }
    }

    // Rarely reached (spurious wake): throw or return nullptr. Return nullptr here.
    return nullptr;
}

void ShmChunkAllocator::retain(void* ptr) noexcept {
    auto idx = indexFromPtr(ptr);
    refCounts_[idx].fetch_add(1, std::memory_order_acq_rel);
}

void ShmChunkAllocator::release(void* ptr) noexcept {
    auto idx = indexFromPtr(ptr);
    uint32_t prev = refCounts_[idx].fetch_sub(1, std::memory_order_acq_rel);
    if (prev == 1) {
        // chunk became free; notify waiting publishers
        std::lock_guard<std::mutex> lock(mtx_);
        cv_free_.notify_one();
    }
}

void ShmChunkAllocator::notifyDataAvailable() noexcept {
    // Wake one or all subscribers waiting for data.
    std::lock_guard<std::mutex> lock(mtx_);
    cv_used_.notify_one();
}

void* ShmChunkAllocator::waitForData() {
    std::unique_lock<std::mutex> lock(mtx_);
    // Wait until any refCount > 0
    cv_used_.wait(lock, [this]() {
        for (std::size_t i = 0; i < capacity_; ++i)
            if (refCounts_[i].load(std::memory_order_acquire) > 0) return true;
        return false;
    });

    // Find a chunk with refcount > 0 and return pointer (caller should retain if needed)
    for (std::size_t i = 0; i < capacity_; ++i) {
        if (refCounts_[i].load(std::memory_order_acquire) > 0) {
            return static_cast<uint8_t*>(chunksBase_) + i * chunkSize_;
        }
    }

    return nullptr; // spurious case
}

std::size_t ShmChunkAllocator::indexFromPtr(void* ptr) const noexcept {
    auto offset = static_cast<uint8_t*>(ptr) - static_cast<uint8_t*>(chunksBase_);
    return offset / chunkSize_;
}
