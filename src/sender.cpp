#include "shm_region.hpp"
#include "shm_chunk_allocator.hpp"
#include "chunk_queue.hpp"
#include "pulse_notifier.hpp"
#include "publisher.hpp"
#include "shm_layout.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include <new>
#include <sys/mman.h>

const bool BENCHMARK_MODE = true; 

int main() {
    const std::string shmName = "/demo_shm";
    
    // 1MB Payload (Like a raw camera frame)
    const int payloadSize = BENCHMARK_MODE ? (1024 * 1024) : 64;
    
    // 1000 Iterations = 1 GB total data transferred
    const int iterations = BENCHMARK_MODE ? 1000 : 100;
    const std::size_t queueCapacity = BENCHMARK_MODE ? 64 : 16;

    // SETUP SHARED MEMORY SIZES -
    std::size_t layoutSize = sizeof(ShmLayout);
    std::size_t queueDataSize = sizeof(std::size_t) * queueCapacity;
    std::size_t allocatorStartOffset = layoutSize + queueDataSize;
    std::size_t poolSize = (queueCapacity + 36) * payloadSize;
    std::size_t totalSize = allocatorStartOffset + poolSize + 65536;

    // cllean up old memory
    shm_unlink(shmName.c_str());

    std::cout << "[SENDER] Creating Shared Memory (" << totalSize / (1024*1024) << " MB)...\n";
    ShmRegion region(shmName, totalSize, ShmRegion::Mode::Create);
    
    uint8_t* base = static_cast<uint8_t*>(region.getBase());
    ShmLayout* layout = reinterpret_cast<ShmLayout*>(base);
    std::size_t* queueBuffer = reinterpret_cast<std::size_t*>(base + sizeof(ShmLayout));

    if (region.isCreator()) {
        new (&layout->header) RegionHeader();
        new (&layout->queue) QueueControlBlock();
    }

    ShmChunkAllocator allocator(region, payloadSize + 32, allocatorStartOffset);
    ChunkQueue queue(&layout->queue, queueBuffer, queueCapacity);
    PulseNotifier notifier;
    Publisher pub(allocator, queue, notifier, &layout->header);

    // this represents the data coming from sensor driver
    std::cout << "[SENDER] Generating " << payloadSize << " bytes of source data...\n";
    std::vector<char> sourceData(payloadSize);
    
    // Fill it with 'A's just so it's not empty
    std::memset(sourceData.data(), 'A', payloadSize);

    std::cout << "[SENDER] Mode: " << (BENCHMARK_MODE ? "REAL-WORLD COPY (memcpy)" : "NORMAL") << "\n";
    std::cout << "[SENDER] Press ENTER to start...\n";
    std::cin.get();

    // --- THE MAIN LOOP ---
    for (int i = 0; i < iterations; ++i) {
        bool sent = false;
        while (!sent) {
            void* ptr = allocator.allocate();
            
            if (ptr) {
                // --- REALISTIC TEST ---
                // we copy FROM our local sensor buffer into shared memory to read and write
                std::memcpy(ptr, sourceData.data(), payloadSize);

                std::size_t idx = allocator.indexFromPtr(ptr);
                if (queue.push(idx)) {
                    notifier.notify(&layout->header);
                    sent = true;
                } else {
                    allocator.release(ptr);
                    std::this_thread::yield(); 
                }
            } else {
                std::this_thread::yield();
            }
        }
    }

    std::cout << "[SENDER] Done. Waiting 5s...\n";
    std::this_thread::sleep_for(std::chrono::seconds(5));
    return 0;
}