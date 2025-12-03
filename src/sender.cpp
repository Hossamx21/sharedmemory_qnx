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
    
    // --- 100 MB TOTAL TEST ---
    
    // 1. Chunk Size: 1 B
    //const int payloadSize = BENCHMARK_MODE ? (1024 * 1024) : 64;
    const int payloadSize = BENCHMARK_MODE ? 100 : 64;
    
    // 2. Iterations: 100 (Total = 100 MB)
    //const int iterations = BENCHMARK_MODE ? 100 : 10;
    const int iterations = BENCHMARK_MODE ? 100 : 10;
    
    
    // 3. Queue Capacity: 16 is plenty (Uses ~16MB RAM)
    //const std::size_t queueCapacity = BENCHMARK_MODE ? 16 : 4; 
    const std::size_t queueCapacity = BENCHMARK_MODE ? 16 : 4;

    // Sizes
    std::size_t layoutSize = sizeof(ShmLayout);
    std::size_t queueDataSize = sizeof(std::size_t) * queueCapacity;
    std::size_t allocatorStartOffset = layoutSize + queueDataSize;
    
    // Allocate pool for Queue + 4 extra chunks for safety
    std::size_t poolSize = (queueCapacity + 4) * payloadSize;
    std::size_t totalSize = allocatorStartOffset + poolSize + 65536;

    // Cleanup
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

    // --- DATA GENERATION ---
    std::cout << "[SENDER] Generating source data (" << payloadSize/1024 << " KB)...\n";
    std::vector<char> sourceData(payloadSize);
    
    // Fill with rolling counter for verification
    uint32_t* sensorData = reinterpret_cast<uint32_t*>(sourceData.data()); //verify
    size_t integerCount = payloadSize / sizeof(uint32_t);

    for (size_t k = 0; k < integerCount; ++k) {
        sensorData[k] = (uint32_t)k; 
    }

    std::cout << "[SENDER] Mode: 100MB TOTAL (1MB x 100)\n";
    std::cout << "[SENDER] Press ENTER to start...\n";
    std::cin.get();

    for (int i = 0; i < iterations; ++i) {
        // Embed Frame ID
        sensorData[0] = i;

        bool sent = false;
        while (!sent) {
            void* ptr = allocator.allocate();
            
            if (ptr) {
                // Real Copy
                std::memcpy(ptr, sourceData.data(), payloadSize); //verify when returning 

                std::size_t idx = allocator.indexFromPtr(ptr);
                if (queue.push(idx)) {
                    bool ok = notifier.notify(&layout->header);
                    if (!ok) {
                        // optional logging or retry
                    }
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