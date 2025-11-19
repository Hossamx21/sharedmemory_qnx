#include "shm_chunk_allocator.hpp"
#include <cstring>     
#include <stdexcept>
#include <system_error>
static constexpr uint32_t MAGIC = 0xCAFEBABE;

ShmChunkAllocator::ShmChunkAllocator(ShmRegion& region, std::size_t chunkSize)
    : region_(region), chunkSize_(chunkSize)
{
    uint8_t* base = static_cast<uint8_t*>(region_.getBase());
    hdr_ = reinterpret_cast<RegionHeader*>(base);

    if (region_.isCreator()) {
        // Creator initializes header
        std::size_t regionSize = region_.getSize();

        std::size_t maxChunks = (regionSize - sizeof(RegionHeader))
                              / (sizeof(ChunkHeader) + chunkSize_);

        capacity_ = maxChunks;

        hdr_->magic       = MAGIC;
        hdr_->capacity    = capacity_;
        hdr_->chunkSize   = chunkSize_;
        hdr_->headerSize  = sizeof(RegionHeader) + capacity_ * sizeof(ChunkHeader);
        hdr_->payloadOffset = hdr_->headerSize;

        // initialize chunk headers
        chunkHdrs_ = reinterpret_cast<ChunkHeader*>(base + sizeof(RegionHeader));
        for (std::size_t i = 0; i < capacity_; ++i) {
            chunkHdrs_[i].refCount.store(0);
            chunkHdrs_[i].inUse.store(0);
        }

    } else {
        // Attacher reads existing metadata
        if (hdr_->magic != MAGIC)
            throw std::runtime_error("Invalid shared memory magic");

        capacity_ = hdr_->capacity;
        chunkSize_ = hdr_->chunkSize;
        chunkHdrs_ = reinterpret_cast<ChunkHeader*>(base + sizeof(RegionHeader));
    }

    payloadBase_ = base + hdr_->payloadOffset;
}

void* ShmChunkAllocator::allocate() noexcept {
    for (std::size_t i = 0; i < capacity_; ++i) {
        uint8_t expectedInUse = 0;
        if (chunkHdrs_[i].inUse.compare_exchange_strong(expectedInUse, 1)) {
            chunkHdrs_[i].refCount.store(1);
            return payloadBase_ + i * chunkSize_;
        }
    }
    return nullptr;
}

void ShmChunkAllocator::retain(void* ptr) noexcept {
    auto idx = indexFromPtr(ptr);
    chunkHdrs_[idx].refCount.fetch_add(1);
}

void ShmChunkAllocator::release(void* ptr) noexcept {
    auto idx = indexFromPtr(ptr);
    uint32_t prev = chunkHdrs_[idx].refCount.fetch_sub(1);
    if (prev == 1) {
        chunkHdrs_[idx].inUse.store(0);
    }
}

std::size_t ShmChunkAllocator::indexFromPtr(void* ptr) const noexcept {
    auto diff = static_cast<uint8_t*>(ptr) - payloadBase_;
    return diff / chunkSize_;
}

void* ShmChunkAllocator::ptrFromIndex(std::size_t idx) const noexcept {
    return payloadBase_ + idx * chunkSize_;
}


