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

// --- CONFIGURATION ---
// Set to true for Speed Test, false for "Hello World" printing
const bool BENCHMARK_MODE = true; 

int main() {
    const std::string shmName = "/demo_shm";
    
    // In Benchmark mode, we need a larger queue and more messages
    const int iterations = BENCHMARK_MODE ? 100000 : 100;
    const std::size_t queueCapacity = BENCHMARK_MODE ? 128 : 16;
    const int payloadSize = BENCHMARK_MODE ? 100 : 64; // 100 bytes for bench

    // Calculate Sizes
    std::size_t layoutSize = sizeof(ShmLayout);
    std::size_t queueDataSize = sizeof(std::size_t) * queueCapacity;
    std::size_t allocatorStartOffset = layoutSize + queueDataSize;
    
    // Total Size: Offset + (Total Payload Data) + Overhead
    std::size_t totalSize = allocatorStartOffset + (iterations * payloadSize) + 65536;

    // 1. CLEANUP (Nuke old memory)
    shm_unlink(shmName.c_str());

    std::cout << "[SENDER] Creating Shared Memory...\n";
    ShmRegion region(shmName, totalSize, ShmRegion::Mode::Create);
    
    uint8_t* base = static_cast<uint8_t*>(region.getBase());
    ShmLayout* layout = reinterpret_cast<ShmLayout*>(base);
    std::size_t* queueBuffer = reinterpret_cast<std::size_t*>(base + sizeof(ShmLayout));

    if (region.isCreator()) {
        new (&layout->header) RegionHeader();
        new (&layout->queue) QueueControlBlock();
    }

    // Initialize Controllers
    ShmChunkAllocator allocator(region, payloadSize + 32, allocatorStartOffset);
    ChunkQueue queue(&layout->queue, queueBuffer, queueCapacity);
    PulseNotifier notifier;
    Publisher pub(allocator, queue, notifier, &layout->header);

    std::cout << "[SENDER] Mode: " << (BENCHMARK_MODE ? "BENCHMARK" : "NORMAL") << "\n";
    std::cout << "[SENDER] Press ENTER to start...\n";
    std::cin.get();

    // --- THE MAIN LOOP ---
    for (int i = 0; i < iterations; ++i) {
        
        // Prepare Data
        char data[128];
        if (BENCHMARK_MODE) {
            // Fill with dummy data 'X' for speed
            std::memset(data, 'X', payloadSize);
        } else {
            // readable string
            std::string s = "Packet " + std::to_string(i);
            std::strcpy(data, s.c_str());
        }

        // SEND LOGIC (Retry Loop)
        bool sent = false;
        while (!sent) {
            // 1. Try to get memory
            void* ptr = allocator.allocate();
            
            if (ptr) {
                // 2. Copy data
                std::memcpy(ptr, data, payloadSize);
                std::size_t idx = allocator.indexFromPtr(ptr);
                
                // 3. Try to push to Queue
                if (queue.push(idx)) {
                    notifier.notify(&layout->header);
                    sent = true; // Success! Exit the retry loop
                    
                    // In Normal mode, print and sleep
                    if (!BENCHMARK_MODE) {
                        std::cout << "[SENDER] Sent: " << data << "\n";
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                } else {
                    // Queue Full! Release memory and wait.
                    allocator.release(ptr);
                    if (!BENCHMARK_MODE) std::cout << "Queue full!\n";
                    std::this_thread::yield(); 
                }
            } else {
                // Allocator Full! Wait.
                std::this_thread::yield();
            }
        }
    }

    std::cout << "[SENDER] Done. Waiting 3s for receiver...\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));
    return 0;
}