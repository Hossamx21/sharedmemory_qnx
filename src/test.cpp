#include "shm_region.hpp"
#include "shm_chunk_allocator.hpp"
#include "chunk_queue.hpp"
#include "pulse_notifier.hpp"
#include "publisher.hpp"
#include "subscriber.hpp"
#include "region_header.hpp"

#include <iostream>
#include <string>
#include <thread>
#include <chrono>

int main() {
    const std::string shmName = "/demo_shm";
    const std::size_t chunkSize = 256;
    // We need enough space for Header + Chunks. 
    // Allocator usually calculates overhead, but let's give it plenty of room (e.g. 64KB)
    const std::size_t totalSize = 65536; 

    // 1. Create the Shared Memory Region (Local Object)
    //    We use Mode::Create because we are the 'server' in this test.
    ShmRegion region(shmName, totalSize, ShmRegion::Mode::Create);

    // 2. Create the Allocator (Local Object)
    //    It takes the region by reference and manages the memory inside it.
    ShmChunkAllocator allocator(region, chunkSize);

    // 3. Create the Queue (Local Object)
    //    Since this is a single-process thread test, a local queue works fine.
    //    Capacity 16 is arbitrary but sufficient for the test.
    ChunkQueue queue(16);

    // 4. Create the Notifier (Local Object)
    PulseNotifier notifier;

    // 5. Get the Header Pointer
    //    The RegionHeader lives at the very start of the shared memory.
    RegionHeader* hdr = static_cast<RegionHeader*>(region.getBase());

    if (!hdr) {
        std::cerr << "Failed to get Region Header. Is SHM initialized?\n";
        return 1;
    }

    // 6. Initialize the Receiver Logic
    //    This sets up the PID/CHID in the header so the Publisher knows where to notify.
    notifier.createReceiver(hdr);

    // 7. Create Publisher and Subscriber
    //    Now we have all the specific ingredients they ask for!
    Publisher pub(allocator, queue, notifier, hdr);
    Subscriber sub(allocator, queue, notifier, hdr);

    // --- Start Logic Threads ---

    // Publisher thread
    std::thread pubThread([&]() {
        for (int i = 0; i < 5; ++i) {
            std::string msg = "message " + std::to_string(i);
            
            // Note: Your header calls this 'publish', not 'publishBlocking'
            // Ensure you pass the size including the null terminator if it's a string
            pub.publish(msg.data(), msg.size() + 1);
            
            std::cout << "[PUB] published: " << msg << '\n';
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    });

    // Subscriber thread
    std::thread subThread([&]() {
        for (int i = 0; i < 5; ++i) {
            void* ptr = sub.receiveBlocking();
            if (ptr) {
                std::cout << "[SUB] received: " << static_cast<char*>(ptr) << '\n';
                
                // Don't forget to release the chunk back to the allocator!
                sub.release(ptr);
            }
            // Add a small delay/yield to let the publisher work
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }
    });

    pubThread.join();
    subThread.join();

    // Clean up is handled by destructors (ShmRegion closes fd, etc.)
    return 0;
}