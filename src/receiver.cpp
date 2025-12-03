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
    
    // MUST MATCH SENDER EXACTLY
    /*const int payloadSize = BENCHMARK_MODE ? (1024 * 1024) : 64;
    const int iterations = BENCHMARK_MODE ? 100 : 10;
    const std::size_t queueCapacity = BENCHMARK_MODE ? 16 : 4;*/
    const int payloadSize = BENCHMARK_MODE ? 100 : 64;
    const int iterations = BENCHMARK_MODE ? 100 : 10; // Expect only 1
    const std::size_t queueCapacity = BENCHMARK_MODE ? 16 : 4;

    std::size_t layoutSize = sizeof(ShmLayout);
    std::size_t queueDataSize = sizeof(std::size_t) * queueCapacity;
    std::size_t allocatorStartOffset = layoutSize + queueDataSize;
    std::size_t poolSize = (queueCapacity + 4) * payloadSize;
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
            int errors = 0;
            auto start = std::chrono::high_resolution_clock::now();
            bool firstMsg = true;

            while (count < iterations) {
                /*every message the Subscriber will call createReceiver(hdr_) and wait() thereafter. However PulseNotifier::createReceiver() returns early if chid_ != -1.
                 But receiveBlocking() calls createReceiver(hdr_) every time, that is redundant*/
                void* ptr = sub.receiveBlocking();
                if (ptr) {
                    if (firstMsg) {
                        start = std::chrono::high_resolution_clock::now();
                        firstMsg = false;
                    }

                    // INTEGRITY CHECK
                    uint32_t* intData = static_cast<uint32_t*>(ptr);
                    
                    if (intData[0] != (uint32_t)count) {
                        std::cerr << "ERROR: Frame mismatch! Exp: " << count << " Got: " << intData[0] << "\n";
                        errors++;
                    }
                    
                    // Check End of Buffer
                    size_t lastIdx = (payloadSize / sizeof(uint32_t)) - 1;
                    if (intData[lastIdx] != (uint32_t)lastIdx) {
                         if (errors < 5) std::cerr << "ERROR: Data corruption at end!\n";
                         errors++;
                    }

                    sub.release(ptr);
                    count++;
                }
            }

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::micro> elapsed = end - start;
            double total_us = elapsed.count();
            
            double throughput = (count / total_us) * 1000000.0;
            double totalBytes = (double)count * payloadSize;
            double mbytesPerSec = (totalBytes / 1024.0 / 1024.0) / (total_us / 1000000.0);

            std::cout << "\n=== 100B TOTAL RESULTS ===\n";
            std::cout << "Count: " << count << "\n";
            std::cout << "Errors: " << errors << "\n";
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