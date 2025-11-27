#include "shm_region.hpp"
#include "shm_chunk_allocator.hpp"
#include "chunk_queue.hpp"
#include "pulse_notifier.hpp"
#include "publisher.hpp"
#include "shm_layout.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <sys/mman.h>
#include <new>

int main() {
    const std::string shmName = "/demo_shm";
    const std::size_t queueCapacity = 16;
    shm_unlink(shmName.c_str()); // This ensures we start with a clean slate, even if the last run crashed.
    
    // Layout: [LayoutStruct] + [QueueDataArray] + [AllocatorPool...]
    std::size_t layoutSize = sizeof(ShmLayout);
    std::size_t queueDataSize = sizeof(std::size_t) * queueCapacity;
    std::size_t allocatorStartOffset = layoutSize + queueDataSize;
    
    // Total Size = Offset + Space for chunks (e.g., 64KB)
    std::size_t totalSize = allocatorStartOffset + 65536;

    std::cout << "[SENDER] Creating Shared Memory...\n";
    ShmRegion region(shmName, totalSize, ShmRegion::Mode::Create);
    
    uint8_t* base = static_cast<uint8_t*>(region.getBase());
    ShmLayout* layout = reinterpret_cast<ShmLayout*>(base);
    std::size_t* queueBuffer = reinterpret_cast<std::size_t*>(base + sizeof(ShmLayout));

    // Initialize Shared Data
    if (region.isCreator()) {
        new (&layout->header) RegionHeader();
        new (&layout->queue) QueueControlBlock();
        // Queue Buffer is raw data, no constructor needed
    }

    // Initialize Controllers
    // Pass 'allocatorStartOffset' so allocator doesn't overwrite the queue!
    ShmChunkAllocator allocator(region, 256, allocatorStartOffset);
    ChunkQueue queue(&layout->queue, queueBuffer, queueCapacity);
    PulseNotifier notifier;

    Publisher pub(allocator, queue, notifier, &layout->header);

    std::cout << "[SENDER] Running...\n";
    
    for (int i = 0; i < 101; ++i) {
        std::string msg = "Packet " + std::to_string(i);
        pub.publish(msg.data(), msg.size() + 1);
        std::cout << "[SENDER] Sent: " << msg << "\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    // Give the receiver 3 seconds to finish reading the queue
    std::this_thread::sleep_for(std::chrono::seconds(3));

    return 0;
}