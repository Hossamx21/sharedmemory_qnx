#include "publisher.hpp"
#include <chrono>
#include <sched.h>

static inline uint64_t getTimeUS() {
    using namespace std::chrono;
    return duration_cast<microseconds>(
        steady_clock::now().time_since_epoch()
    ).count();
}
Publisher::Publisher(ShmChunkAllocator& allocator, RegionHeader* hdr)
    : allocator_(allocator),
      hdr_(hdr)
{
    // Initialize cache with -1 (meaning "not connected")
    coids_.resize(RegionHeader::MAX_SUBSCRIBERS, -1);
}

Publisher::~Publisher() {
    for (int coid : coids_) {
        if (coid != -1) ConnectDetach(coid);
    }
}
/*void Publisher::publish(const void* data, std::size_t len) {
    void* ptr = allocator_.allocate();
    if (!ptr)
        return; 

    if (len > allocator_.getChunkSize()) {
    len = allocator_.getChunkSize(); // or return false
    }

    std::memcpy(ptr, data, len);

    std::size_t idx = allocator_.indexFromPtr(ptr);

    // --- ensure we snapshot active subscribers BEFORE we add refs or notify ---
    uint32_t subs = countActiveSubscribers();

    if (subs == 0) {
        // No subscribers: don't keep the chunk for 'latest' semantics.
        // Free this chunk immediately to avoid leak.
        allocator_.release(ptr);
        return; // nothing to notify
    }

    // Add subscriber refs to the allocated chunk (adds subs refs to current refcount=1)
    if (subs > 0) {
        allocator_.setInitialRefCount(idx, subs);
    }

    hdr_->latest_idx.store(idx, std::memory_order_release);
    hdr_->latest_ts_us.store(getTimeUS(), std::memory_order_release);
    hdr_->latest_seq.fetch_add(1, std::memory_order_release);

     // Notify subscribers AFTER refs and metadata are in place
    notifyAllSubscribers();

    // Publisher should drop its own reference if subscribers exist.
    // We allocated earlier which gave us one reference; subscribers hold 'subs' refs now.
    // If subs > 0, release the publisher-owned reference so only subscribers hold it.
    // If subs == 0, keep publisher's ref so latest_idx remains valid.
    if (subs > 0) {
        allocator_.release(ptr);
    }


    /*if (queue_.push(idx)) {
        notifier_.notify(hdr_);
    } else {
        allocator_.release(ptr); /*The message cannot be delivered now. Free the chunk:*/
    
    void Publisher::publish(const void* data, std::size_t len) {
    // 1. Allocate Memory (Standard)
    void* ptr = allocator_.allocate();
    if (!ptr) {
        // Allocator is full. In a real system, you might log an error here.
        return; 
    }

    if (len > allocator_.getChunkSize()) {
        len = allocator_.getChunkSize(); 
    }

    // 2. Copy Data (Zero-Copy Preparation)
    std::memcpy(ptr, data, len);
    // 3. Prepare for Multicast
    // We have 1 reference from allocate().
    // If we have N queues, we need N references total.
    // So we must retain() (N - 1) times.
    for (size_t i = 1; i < queues_.size(); ++i) {
        allocator_.retain(ptr);
    }
    std::size_t newIdx = allocator_.indexFromPtr(ptr);
    // 4. Push to ALL Queues
    for (ChunkQueue* q : queues_) {
        // Try to push. If full, force-pop the oldest item.
        while (!q->push(newIdx)) {
            std::size_t oldIdx;
            
            // Force remove the oldest item to make room (Ring Buffer behavior)
            if (q->pop(oldIdx)) {
                // We stole 'oldIdx' from the queue. 
                // The subscriber will never see it, so we must release its memory.
                allocator_.release(allocator_.ptrFromIndex(oldIdx));
            } else {
                // Queue reported full, but pop failed? (Rare race condition).
                sched_yield();
            }
        }
    }
    // We try to push the new index to the queue.
    // If push() returns false (Queue Full), we enter the loop.
    /*while (!queue_.push(newIdx)) {
        
        std::size_t oldIdx; //it gets filled in ChunkQueue::pop
        
        // We force-pop (remove) the oldest item to make room.
        if (queue_.pop(oldIdx)) {
            // CLEANUP: We stole 'oldIdx' back from the queue.
            // Since the Subscriber never read it, we must free it.
            allocator_.release(allocator_.ptrFromIndex(oldIdx));
        } else {
            // Queue was full, but pop failed? (Race condition).
            // Yield CPU and try again.
            std::this_thread::yield();
        }
    }*/
    // 5. Notify
    notifyAllSubscribers();
}


void Publisher::addQueue(ChunkQueue* queue) {
    if (queue) {
        queues_.push_back(queue);
    }
}

void Publisher::notifyAllSubscribers() {
    // Pulse code must match what Subscriber expects (usually 1)
    const int PULSE_CODE = 1; 

    for (uint32_t i = 0; i < RegionHeader::MAX_SUBSCRIBERS; ++i) {
        const auto &s = hdr_->subscribers[i];
        
        // 1. If subscriber is inactive, clean up connection
        if (!s.active.load()) {
            if (coids_[i] != -1) {
                ConnectDetach(coids_[i]);
                coids_[i] = -1;
            }
            continue;
        }

        // 2. If active but not connected, Connect now 
        if (coids_[i] == -1) {
            int pid = s.pid.load();
            int chid = s.chid.load();
            
            if (pid != 0 && chid != 0) {
                coids_[i] = ConnectAttach(ND_LOCAL_NODE, pid, chid, _NTO_SIDE_CHANNEL, 0);
            }
        }

        // 3. Send Pulse 
        if (coids_[i] != -1) {
            MsgSendPulse(coids_[i], SIGEV_PULSE_PRIO_INHERIT, PULSE_CODE, 0);
        }
    }
}


/*uint32_t Publisher::countActiveSubscribers() const {
    uint32_t c = 0;
    for (uint32_t i = 0; i < RegionHeader::MAX_SUBSCRIBERS; ++i)
        if (hdr_->subscribers[i].active.load()) c++;
    return c;
}*/


