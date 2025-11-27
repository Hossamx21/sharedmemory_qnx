#include "shm_chunk_allocator.hpp"
#include <cstring>
#include <stdexcept>

static constexpr uint32_t MAGIC = 0xCAFEBABE;
ShmChunkAllocator::ShmChunkAllocator(ShmRegion& region, std::size_t chunkSize, std::size_t startOffset)
    : region_(region), chunkSize_(chunkSize)
{
    uint8_t* base = static_cast<uint8_t*>(region_.getBase());
    hdr_ = reinterpret_cast<RegionHeader*>(base);

    if (region_.isCreator()) {
        std::size_t regionSize = region_.getSize();
        
        // Calculate capacity based on remaining space after the Offset
        // available = Total - (Queue/Layout Stuff)
        if (startOffset >= regionSize) throw std::runtime_error("Offset exceeds region size");
        
        std::size_t available = regionSize - startOffset;
        std::size_t maxChunks = available / (sizeof(ChunkHeader) + chunkSize_);

        capacity_ = maxChunks;

        hdr_->magic       = MAGIC;
        hdr_->capacity    = capacity_;
        hdr_->chunkSize   = chunkSize_;
        // The payload (actual data) starts after all the headers
        hdr_->payloadOffset = startOffset + (capacity_ * sizeof(ChunkHeader));
        
        // Stash the offset so we know where headers begin
        hdr_->headerSize = startOffset; 

        // Place Chunk Headers starting at the offset
        chunkHdrs_ = reinterpret_cast<ChunkHeader*>(base + startOffset);
        
        for (std::size_t i = 0; i < capacity_; ++i) {
            chunkHdrs_[i].refCount.store(0);
            chunkHdrs_[i].inUse.store(0);
        }
    } else {
        if (hdr_->magic != MAGIC) throw std::runtime_error("Invalid shared memory magic");
        
        capacity_ = hdr_->capacity;
        chunkSize_ = hdr_->chunkSize;
        // Reader uses the stored offset to find headers
        chunkHdrs_ = reinterpret_cast<ChunkHeader*>(base + hdr_->headerSize);
    }

    payloadBase_ = base + hdr_->payloadOffset;
}

// ... (Rest of functions: allocate, retain, release, indexFromPtr, ptrFromIndex remain the same) ...
void* ShmChunkAllocator::allocate() noexcept {
    for (std::size_t i = 0; i < capacity_; ++i) {
        uint8_t expected = 0;
        if (chunkHdrs_[i].inUse.compare_exchange_strong(expected, 1)) {
            chunkHdrs_[i].refCount.store(1);
            return payloadBase_ + i * chunkSize_;
        }
    }
    return nullptr;
}

void ShmChunkAllocator::retain(void* ptr) noexcept {
    auto idx = indexFromPtr(ptr);
    if (idx < capacity_) chunkHdrs_[idx].refCount.fetch_add(1);
}

void ShmChunkAllocator::release(void* ptr) noexcept {
    auto idx = indexFromPtr(ptr);
    if (idx < capacity_) {
        uint32_t prev = chunkHdrs_[idx].refCount.fetch_sub(1);
        if (prev == 1) chunkHdrs_[idx].inUse.store(0);
    }
}

std::size_t ShmChunkAllocator::indexFromPtr(void* ptr) const noexcept {
    return (static_cast<uint8_t*>(ptr) - payloadBase_) / chunkSize_;
}

void* ShmChunkAllocator::ptrFromIndex(std::size_t idx) const noexcept {
    return payloadBase_ + idx * chunkSize_;
}