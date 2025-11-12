#pragma once
#include "shm_region.hpp"
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

class ShmChunkAllocator {
public:
    ShmChunkAllocator(ShmRegion& region, std::size_t chunkSize);

    // Non-blocking: returns nullptr if no free chunk.
    void* allocate() noexcept;

    // Blocking: waits until a free chunk is available and returns it.
    // Throws std::system_error on interruption/serious error (optional).
    void* allocateBlocking();

    // Mark that a user has an additional reference to the chunk.
    void retain(void* ptr) noexcept;

    // Decrement reference count; when goes to zero the chunk becomes free and notifies producers.
    void release(void* ptr) noexcept;

    // Called by publisher after writing to chunk to wake subscribers.
    void notifyDataAvailable() noexcept;

    // Blocking call used by subscribers to wait for any chunk with refCount > 0.
    // Returns pointer to first chunk with refCount > 0, or nullptr if none (should be rare).
    void* waitForData();

    std::size_t capacity() const noexcept { return capacity_; }
    std::size_t chunkSize() const noexcept { return chunkSize_; }

private:
    std::size_t indexFromPtr(void* ptr) const noexcept;

    ShmRegion& region_;
    std::size_t chunkSize_;
    std::size_t capacity_;
    void* chunksBase_;

    std::vector<std::atomic<uint32_t>> refCounts_;

    mutable std::mutex mtx_;
    std::condition_variable cv_free_; // signalled when a chunk becomes free
    std::condition_variable cv_used_; // signalled when a chunk becomes used (data)
};
