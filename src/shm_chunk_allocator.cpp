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
        std::size_t maxChunks = available / (sizeof(ChunkHeader) + chunkSize_); //store all ChunkHeaders first (capacity_sizeof(header)) then contiguous payload area (capacity_chunkSize_)

        capacity_ = maxChunks;

        hdr_->magic       = MAGIC;
        hdr_->capacity    = capacity_;
        hdr_->chunkSize   = chunkSize_;
        // The payload (actual data) starts after all the headers
        hdr_->payloadOffset = startOffset + (capacity_ * sizeof(ChunkHeader));
        
        // Stash the offset so we know where headers begin
        hdr_->headerSize = startOffset;  //odd naming — earlier we assumed headerSize == size of the header region

        // Place Chunk Headers starting at the offset
        chunkHdrs_ = reinterpret_cast<ChunkHeader*>(base + startOffset); //Potential integer overflow: capacity_*sizeof(ChunkHeader) or startOffset + (...) can overflow std::size_t
        
        for (std::size_t i = 0; i < capacity_; ++i) {
            chunkHdrs_[i].state.store(0);
        }
    } else {
        if (hdr_->magic != MAGIC) throw std::runtime_error("Invalid shared memory magic");
        
        capacity_ = hdr_->capacity;
        chunkSize_ = hdr_->chunkSize; //overwrites chunkSize_ passed in constructor
        // Reader uses the stored offset to find headers
        chunkHdrs_ = reinterpret_cast<ChunkHeader*>(base + hdr_->headerSize);
    }

    payloadBase_ = base + hdr_->payloadOffset;
}
/*payloadBase_ + capacity_*chunkSize_ <= regionSize (no out-of-bounds).
hdr_->payloadOffset > startOffset + capacity_ * sizeof(ChunkHeader)? Actually computed equal; but you must ensure payload doesn't overlap headers.
chunkSize_ must be large enough to hold the data required and alignment requirements.*/

// ... (Rest of functions: allocate, retain, release, indexFromPtr, ptrFromIndex remain the same) ...
void* ShmChunkAllocator::allocate() noexcept {
    //suggests a better design: store free chunk indices in the queue so allocate() pop()s a free index (fast) and release() push()es index back — this avoids O(N) scan and race complexity
    for (std::size_t i = 0; i < capacity_; ++i) {
       uint32_t expected = 0;
       uint32_t desired  = 3; // inUse=1, refcount=1
        if (chunkHdrs_[i].state.compare_exchange_strong(expected, desired)) {
            return payloadBase_ + i * chunkSize_;
        }
    }
    return nullptr;
}

void ShmChunkAllocator::retain(void* ptr) noexcept {
    auto idx = indexFromPtr(ptr);
    if (idx < capacity_) {
        // add 1 to refcount -> encoded as +2 in the state word
        chunkHdrs_[idx].state.fetch_add((1u << 1), std::memory_order_acq_rel);
    }
}


void ShmChunkAllocator::release(void* ptr) noexcept {
    auto idx = indexFromPtr(ptr);
    if (idx >= capacity_) return;

    // subtract one ref (2 in encoded units)
    uint32_t prev = chunkHdrs_[idx].state.fetch_sub((1u << 1), std::memory_order_acq_rel);
    uint32_t prevRefs = prev >> 1;
    if (prevRefs == 1u) {
        // after decrement, refcount is zero — try to clear inUse bit
        uint32_t expected = (0u << 1) | 1u; // refcount=0, inUse=1 -> value == 1
        chunkHdrs_[idx].state.compare_exchange_strong(expected, 0u,
            std::memory_order_acq_rel, std::memory_order_relaxed);
    }
}


std::size_t ShmChunkAllocator::indexFromPtr(void* ptr) const noexcept {
    if (!ptr) return capacity_; // invalid guard
    std::ptrdiff_t diff = static_cast<uint8_t*>(ptr) - payloadBase_; // invalid guard
    if (diff < 0) return capacity_;
    std::size_t idx = static_cast<std::size_t>(diff) / chunkSize_;
    if (idx >= capacity_) return capacity_;
    return idx;
}

void* ShmChunkAllocator::ptrFromIndex(std::size_t idx) const noexcept {
    if (idx >= capacity_) return nullptr;
    return payloadBase_ + idx * chunkSize_;
}

void ShmChunkAllocator::setInitialRefCount(std::size_t idx, uint32_t subs) {
    if (idx >= capacity_) return;
    if (subs == 0) return; // nothing to add

    // encoded units: refcount is stored shifted by 1 => add subs << 1
    uint32_t delta = (subs << 1);
    chunkHdrs_[idx].state.fetch_add(delta, std::memory_order_acq_rel);
}
