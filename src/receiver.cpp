#include "shm_region.hpp"
#include "shm_chunk_allocator.hpp"
#include "chunk_queue.hpp"
#include "pulse_notifier.hpp"
#include "subscriber.hpp"
#include "shm_layout.hpp"
#include <iostream>
#include <thread>
#include <chrono>

const bool BENCHMARK_MODE = true; 

int main() {
    const std::string shmName = "/demo_shm";
    // MUST MATCH SENDER
    const int payloadSize = BENCHMARK_MODE ? (1024 * 1024) : 64;
    const int iterations = BENCHMARK_MODE ? 1000 : 100;
    const std::size_t queueCapacity = BENCHMARK_MODE ? 64 : 16;

    std::size_t layoutSize = sizeof(ShmLayout);
    std::size_t queueDataSize = sizeof(std::size_t) * queueCapacity;
    std::size_t allocatorStartOffset = layoutSize + queueDataSize;
    std::size_t poolSize = (queueCapacity + 36) * payloadSize;
    std::size_t totalSize = allocatorStartOffset + poolSize + 65536;

    std::cout << "[RECEIVER] Waiting for Sender...\n";

    while (true) {
        try {
            ShmRegion region(shmName, totalSize, ShmRegion::Mode::Attach);
            uint8_t* base = static_cast<uint8_t*>(region.getBase());
            ShmLayout* layout = reinterpret_cast<ShmLayout*>(base);
            std::size_t* queueBuffer = reinterpret_cast<std::size_t*>(base + sizeof(ShmLayout));

            ShmChunkAllocator allocator(region, payloadSize + 32, allocatorStartOffset);
            ChunkQueue queue(&layout->queue, queueBuffer, queueCapacity);
            PulseNotifier notifier;
            Subscriber sub(allocator, queue, notifier, &layout->header);

            std::cout << "[RECEIVER] Ready!\n";

            int count = 0;
            int errors = 0; // Track data integrity errors
            auto start = std::chrono::high_resolution_clock::now();
            bool firstMsg = true;

            while (count < iterations) {
                void* ptr = sub.receiveBlocking();
                if (ptr) {
                    if (firstMsg) {
                        start = std::chrono::high_resolution_clock::now();
                        firstMsg = false;
                    }

                    // read the first integer 
                    uint32_t* intData = static_cast<uint32_t*>(ptr);
                    
                    if (intData[0] != (uint32_t)count) {
                        // only print first few errors to avoid spam
                        if (errors < 5) {
                            std::cerr << "ERROR: Expected Frame " << count << " but got " << intData[0] << "\n";
                        }
                        errors++;
                    }
                    // -----------------------

                    sub.release(ptr);
                    count++;
                }
            }

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::micro> elapsed = end - start;
            double total_us = elapsed.count();
            double throughput = (count / total_us) * 1000000.0;
            
            // calculate Data Rate (MB/s)
            double totalBytes = (double)count * payloadSize;
            double mbytesPerSec = (totalBytes / 1024.0 / 1024.0) / (total_us / 1000000.0);

            std::cout << "\n=== 1MB PAYLOAD RESULTS ===\n";
            std::cout << "Count: " << count << "\n";
            std::cout << "Errors: " << errors << (errors == 0 ? " (PERFECT)" : " (CORRUPT!!!!!)") << "\n";
            std::cout << "Time:  " << total_us / 1000.0 << " ms\n";
            std::cout << "Speed: " << throughput << " msgs/sec\n";
            std::cout << "Bandwidth: " << mbytesPerSec << " MB/s\n";
            
            return 0;

        } catch (...) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    return 0;
}