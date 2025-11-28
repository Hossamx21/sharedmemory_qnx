#include "shm_region.hpp"
#include "shm_chunk_allocator.hpp"
#include "chunk_queue.hpp"
#include "pulse_notifier.hpp"
#include "subscriber.hpp"
#include "shm_layout.hpp"
#include <iostream>
#include <thread>
#include <chrono>

// MUST MATCH SENDER
const bool BENCHMARK_MODE = true; 

int main() {
    const std::string shmName = "/demo_shm";
    const int iterations = BENCHMARK_MODE ? 100000 : 100;
    const std::size_t queueCapacity = BENCHMARK_MODE ? 128 : 16;
    const int payloadSize = BENCHMARK_MODE ? 100 : 64;

    // Calculation must match Sender 
    std::size_t layoutSize = sizeof(ShmLayout);
    std::size_t queueDataSize = sizeof(std::size_t) * queueCapacity;
    std::size_t allocatorStartOffset = layoutSize + queueDataSize;
    std::size_t totalSize = allocatorStartOffset + (iterations * payloadSize) + 65536;

    std::cout << "[RECEIVER] Waiting for Sender...\n";

    while (true) {
        try {
            // Attach logic (Same as before)
            ShmRegion region(shmName, totalSize, ShmRegion::Mode::Attach);
            uint8_t* base = static_cast<uint8_t*>(region.getBase());
            ShmLayout* layout = reinterpret_cast<ShmLayout*>(base);
            std::size_t* queueBuffer = reinterpret_cast<std::size_t*>(base + sizeof(ShmLayout));

            ShmChunkAllocator allocator(region, payloadSize + 32, allocatorStartOffset);
            ChunkQueue queue(&layout->queue, queueBuffer, queueCapacity);
            PulseNotifier notifier;
            Subscriber sub(allocator, queue, notifier, &layout->header);

            std::cout << "[RECEIVER] Ready!\n";

            // STATS VARIABLES
            int count = 0;
            auto start = std::chrono::high_resolution_clock::now();
            bool firstMsg = true;

            // RECEIVE LOOP
            while (count < iterations) {
                void* ptr = sub.receiveBlocking();
                
                if (ptr) {
                    // Start timer when FIRST packet arrives
                    if (firstMsg) {
                        start = std::chrono::high_resolution_clock::now();
                        firstMsg = false;
                    }

                    if (BENCHMARK_MODE) {
                        // In Benchmark, we don't print (printing is slow)
                        // Just count it.
                    } else {
                        // Normal mode: Print content
                        std::cout << "[RECEIVER] Got: " << static_cast<char*>(ptr) << "\n";
                    }

                    sub.release(ptr);
                    count++;
                }
            }

            // REPORT RESULTS
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::micro> elapsed = end - start;
            double total_us = elapsed.count();
            double throughput = (count / total_us) * 1000000.0;

            std::cout << "\n=== RESULTS ===\n";
            std::cout << "Count: " << count << "\n";
            std::cout << "Time:  " << total_us / 1000.0 << " ms\n";
            std::cout << "Speed: " << throughput << " msgs/sec\n";
            
            return 0; // Exit after test

        } catch (...) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    return 0;
}