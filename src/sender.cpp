#include "shm_region.hpp"
#include "shm_chunk_allocator.hpp"
#include "chunk_queue.hpp"
#include "publisher.hpp"
#include "shm_layout.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include <new>
#include <iomanip>

// Helper to print hex
void printHex(const char* label, const uint8_t* data, size_t len) {
    std::cout << label << " [ ";
    for (size_t i = 0; i < len; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                  << (int)data[i] << " ";
    }
    std::cout << std::dec << "]\n";
}

int main() {
    const std::string shmName = "/demo_shm";
    const int payloadSize = 100; 
    const int iterations = 100;   
    const std::size_t capLogger = 16; 
    const std::size_t capAD     = 2; 

    // --- MEMORY LAYOUT ---
    std::size_t layoutSize = sizeof(ShmLayout);
    std::size_t bufSizeLog = sizeof(std::size_t) * capLogger;
    std::size_t bufSizeAD  = sizeof(std::size_t) * capAD;
    std::size_t offsetLog = layoutSize;
    std::size_t offsetAD  = layoutSize + bufSizeLog;
    std::size_t offsetAlloc = layoutSize + bufSizeLog + bufSizeAD;
    std::size_t poolSize = (capLogger + capAD + 8) * payloadSize; 
    std::size_t totalSize = offsetAlloc + poolSize + 65536;

    shm_unlink(shmName.c_str());

    std::cout << "[SENDER] Creating Shared Memory...\n";
    ShmRegion region(shmName, totalSize, ShmRegion::Mode::Create);
    
    uint8_t* base = static_cast<uint8_t*>(region.getBase());
    ShmLayout* layout = reinterpret_cast<ShmLayout*>(base);
    std::size_t* bufferLog = reinterpret_cast<std::size_t*>(base + offsetLog);
    std::size_t* bufferAD  = reinterpret_cast<std::size_t*>(base + offsetAD);

    if (region.isCreator()) {
        new (&layout->header) RegionHeader();
        new (&layout->queueLogger) QueueControlBlock();
        new (&layout->queueAD)     QueueControlBlock();
    }

    ShmChunkAllocator allocator(region, payloadSize, offsetAlloc);
    ChunkQueue queueLogger(&layout->queueLogger, bufferLog, capLogger);
    ChunkQueue queueAD(&layout->queueAD, bufferAD, capAD);
    Publisher pub(allocator, &layout->header);

    pub.addQueue(&queueLogger);
    pub.addQueue(&queueAD);

    std::cout << "[SENDER] Press ENTER to start...\n";
    std::cin.get();

    std::vector<uint8_t> buffer(payloadSize);

    /// --- BENCHMARK START ---
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        // 1. Fill Header (Frame ID)
        uint32_t* header = reinterpret_cast<uint32_t*>(buffer.data());
        header[0] = i; 

        // 2. Fill Data Pattern (start after the 4-byte Frame ID)
        for (int k = 4; k < payloadSize; ++k) {
            buffer[k] = (uint8_t)((k + i) % 255); // Simple pattern
        }
        
        pub.publish(buffer.data(), payloadSize);

        // Visual check of first 16 bytes
        std::cout << "[SENDER] >>> Sent Frame " << i << ": ";
        printHex("", buffer.data(), 16);
        
        //std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // --- BENCHMARK END ---
    auto end = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double> diff = end - start;
    double seconds = diff.count();
    
    double totalMB = (double)(iterations * payloadSize) / (1024.0 * 1024.0);
    double bandwidth = totalMB / seconds;
    double msgsPerSec = iterations / seconds;

    std::cout << "\n[RESULT] Finished!\n";
    std::cout << "------------------------------------------\n";
    std::cout << " Time Elapsed: " << std::fixed << std::setprecision(4) << seconds << " s\n";
    std::cout << " Total Data:   " << totalMB << " MB\n";
    std::cout << " Bandwidth:    " << bandwidth << " MB/s\n";
    std::cout << " Speed:        " << msgsPerSec << " Frames/sec\n";
    std::cout << "------------------------------------------\n";
    std::cout << "[SENDER] Done.\n";

    std::this_thread::sleep_for(std::chrono::seconds(5));
    return 0;
}