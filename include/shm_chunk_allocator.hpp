#pragma once
#include "shm_region.hpp"
#include "region_header.hpp"
#include "chunk_header.hpp"
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>
#include <cstring>     
#include <stdexcept>
#include <system_error>

class ShmChunkAllocator {
public:
    ShmChunkAllocator(ShmRegion& region, std::size_t chunkSize, std::size_t startOffset);

    // Non-blocking: returns nullptr if no free chunk.
    void* allocate() noexcept;

    std::size_t getChunkSize() const noexcept { return chunkSize_; }


    // Mark that a user has an additional reference to the chunk.
    void  retain(void* ptr) noexcept;

    // Decrement reference count; when goes to zero the chunk becomes free and notifies producers.
    void  release(void* ptr) noexcept;

    std::size_t indexFromPtr(void* ptr) const noexcept;

    void* ptrFromIndex(std::size_t idx) const noexcept;

    void setInitialRefCount(std::size_t idx, uint32_t subscribers);


    

private:
    ShmRegion& region_;

    RegionHeader* hdr_;
    ChunkHeader* chunkHdrs_;
    uint8_t* payloadBase_;

    std::size_t capacity_;
    std::size_t chunkSize_;
};
