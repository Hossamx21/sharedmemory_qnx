#include "shm_region.hpp"
#include "shm_chunk_allocator.hpp"
#include "chunk_queue.hpp"
#include "pulse_notifier.hpp"
#include "subscriber.hpp"
#include "shm_layout.hpp"
#include <iostream>
#include <thread>
#include <iomanip>

void verifyAndPrint(const void* ptr, int size) {
    const uint8_t* data = static_cast<const uint8_t*>(ptr);
    const uint32_t* header = static_cast<const uint32_t*>(ptr);
    uint32_t frameId = header[0];

    bool error = false;
    for (int k = 4; k < size; ++k) {
        uint8_t expected = (uint8_t)((k + frameId) % 255);
        if (data[k] != expected) {
            error = true;
            break;
        }
    }

    std::cout << "        [AD] <<< Frame " << frameId << ": ";
    std::cout << "[ ";
    for(int i=0; i<8; ++i) 
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)data[i] << " ";
    std::cout << std::dec << "] ";

    if (error) std::cout << " (DATA ERROR!)\n";
    else       std::cout << " (Data OK)\n";
}

int main() {
    const std::string shmName = "/demo_shm";
    const int payloadSize = 100;
    const std::size_t capLogger = 16;
    const std::size_t capAD     = 2;
    const int iterations = 100;

    std::size_t layoutSize = sizeof(ShmLayout);
    std::size_t bufSizeLog = sizeof(std::size_t) * capLogger;
    std::size_t bufSizeAD  = sizeof(std::size_t) * capAD;
    std::size_t offsetAD  = layoutSize + bufSizeLog;
    std::size_t offsetAlloc = layoutSize + bufSizeLog + bufSizeAD;
    std::size_t poolSize = (capLogger + capAD + 8) * payloadSize; 
    std::size_t totalSize = offsetAlloc + poolSize + 65536;

    

    while (true) {
        try {
            ShmRegion region(shmName, totalSize, ShmRegion::Mode::Attach);
            uint8_t* base = static_cast<uint8_t*>(region.getBase());
            ShmLayout* layout = reinterpret_cast<ShmLayout*>(base);
            std::size_t* bufferAD = reinterpret_cast<std::size_t*>(base + offsetAD);

            ShmChunkAllocator allocator(region, payloadSize, offsetAlloc);
            ChunkQueue queue(&layout->queueAD, bufferAD, capAD);
            PulseNotifier notifier;
            Subscriber sub(allocator, queue, notifier, &layout->header);

            std::cout << "[AD] Ready.\n";

            std::cout << "[AD-SYSTEM] Waiting...\n";
           // --- BENCHMARK VARIABLES ---
            bool timing = false;
            auto start = std::chrono::high_resolution_clock::now();
            int framesProcessed = 0;

            while (true) {
                void* ptr = sub.receiveBlocking();
                if (ptr) {
                    if (!timing) {
                        start = std::chrono::high_resolution_clock::now();
                        timing = true;
                        framesProcessed = 0;
                    }
                    verifyAndPrint(ptr, payloadSize);
                    framesProcessed++;
                    
                    // SLOW PROCESSING
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    // Check for LAST frame
                    uint32_t* header = static_cast<uint32_t*>(ptr);
                    uint32_t currentId = header[0];

                    if (currentId >= iterations - 1) {
                        auto end = std::chrono::high_resolution_clock::now();
                        std::chrono::duration<double> diff = end - start;
                        double seconds = diff.count();
                        if (seconds < 0.000001) seconds = 0.000001;

                        // Calculate effective processing speed
                        double speed = framesProcessed / seconds;

                        std::cout << "\n[AD RESULT] Finished Batch!\n";
                        std::cout << "------------------------------------------\n";
                        std::cout << " Processed: " << framesProcessed << " / " << iterations << " (Dropped many)\n";
                        std::cout << " Time:      " << seconds << " s\n";
                        std::cout << " Speed:     " << speed << " Frames/sec\n";
                        std::cout << "------------------------------------------\n";
                        timing = false;
                    }
                    sub.release(ptr);
                }
            }
        } catch (...) { std::this_thread::sleep_for(std::chrono::seconds(1)); }
    }
}