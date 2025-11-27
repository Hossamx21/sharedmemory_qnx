#include "shm_region.hpp"
#include "shm_chunk_allocator.hpp"
#include "chunk_queue.hpp"
#include "pulse_notifier.hpp"
#include "subscriber.hpp"
#include "shm_layout.hpp"
#include <iostream>
#include <thread>

int main() {
    const std::string shmName = "/demo_shm";
    const std::size_t queueCapacity = 16;
    
    // Exact same math as Sender to find pointers
    std::size_t layoutSize = sizeof(ShmLayout);
    std::size_t queueDataSize = sizeof(std::size_t) * queueCapacity;
    std::size_t allocatorStartOffset = layoutSize + queueDataSize;
    std::size_t totalSize = allocatorStartOffset + 65536;

    std::cout << "[RECEIVER] Connecting...\n";

    // Retry loop to wait for Sender
    while (true) {
        try {
            ShmRegion region(shmName, totalSize, ShmRegion::Mode::Attach);
            uint8_t* base = static_cast<uint8_t*>(region.getBase());
            ShmLayout* layout = reinterpret_cast<ShmLayout*>(base);
            std::size_t* queueBuffer = reinterpret_cast<std::size_t*>(base + sizeof(ShmLayout));

            // Create Controllers
            ShmChunkAllocator allocator(region, 256, allocatorStartOffset);
            ChunkQueue queue(&layout->queue, queueBuffer, queueCapacity);
            PulseNotifier notifier;

            Subscriber sub(allocator, queue, notifier, &layout->header);

            std::cout << "[RECEIVER] Ready!\n";

            while (true) {
                void* ptr = sub.receiveBlocking();
                if (ptr) {
                    std::cout << "[RECEIVER] Data: " << static_cast<char*>(ptr) << "\n";
                    sub.release(ptr);
                }
            }
        } catch (...) {
            std::cout << "Waiting for Sender...\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    return 0;
}