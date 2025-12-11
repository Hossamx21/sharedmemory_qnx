#include "shm_region.hpp"
#include "pulse_notifier.hpp"
#include "shm_layout.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <iomanip>
#include <sched.h>

// Helper to register simple subscribers in the header table
void registerSelf(RegionHeader* hdr, PulseNotifier& notifier) {
    notifier.createReceiver(nullptr);
    int pid = getpid();
    int chid = notifier.getChid();
    for (int i=0; i<2; ++i) {
        bool exp = false;
        // Find a slot that is NOT active and atomically claim it
        if (hdr->subscribers[i].active.compare_exchange_strong(exp, true)) {
            hdr->subscribers[i].pid = pid;
            hdr->subscribers[i].chid = chid;
            return;
        }
    }
}

// Verification function: Checks if data matches the expected pattern
void printAndVerify(const std::string& name, void* ptr) {
    uint8_t* d = static_cast<uint8_t*>(ptr);
    uint32_t* h = reinterpret_cast<uint32_t*>(ptr);
    uint32_t fid = h[0];

    // Verify Pattern: d[k] must equal (k + fid) % 255
    bool err = false;
    for(int k=4; k<1024; ++k) {
        if (d[k] != (uint8_t)((k + fid) % 255)) {
            err = true;
            break;
        }
    }

    std::cout << "[" << name << "] Frame " << fid << " ";
    std::cout << "[ ";
    // Print first 16 bytes
    for(int i=0; i<16; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)d[i] << " ";
    }
    std::cout << std::dec << "] " << (err ? "ERROR" : "OK") << "\n";
}

int main(/*int argc, char* argv[]*/) {
    // Name the process based on argument (e.g., ./sub SPEED)
    //std::string name = (argc > 1) ? argv[1] : "SUB";
    std::string name = "SPEED_CONTROLLER";
    
    const std::string shmName = "/demo_sync_shm";
    std::size_t totalSize = sizeof(shm_layout) + (FRAME_SIZE * 2);

    std::cout << "[" << name << "] Waiting for SHM...\n";
    while (true) {
        try {
            ShmRegion region(shmName, totalSize, ShmRegion::Mode::Attach);
            uint8_t* base = static_cast<uint8_t*>(region.getBase());
            shm_layout* layout = reinterpret_cast<shm_layout*>(base);
            
            uint8_t* buffer0 = base + sizeof(shm_layout);
            uint8_t* buffer1 = buffer0 + FRAME_SIZE;

            PulseNotifier notifier;
            registerSelf(&layout->header, notifier);

            std::cout << "[" << name << "] Ready. Waiting for pulses...\n";

            while (true) {
                // 1. Wait for Signal from Publisher
                notifier.wait(); 

                // 2. Find which buffer has the new data
                int idx = layout->currentBufferIdx.load(std::memory_order_acquire);
                uint8_t* data = (idx == 0) ? buffer0 : buffer1;
                uint32_t* header = reinterpret_cast<uint32_t*>(data);

                // 3. Simulate Control Logic Processing (Speed/Steering)
                // We sleep 10ms to simulate heavy math.
                std::this_thread::sleep_for(std::chrono::milliseconds(10));

                // 4. Print verification every 50 frames (1 second)
                /*if (header[0] % 50 == 0) {
                    
                }*/
                printAndVerify(name, data);

                // 5. Acknowledge Completion
                // Increment 'finishedCount' so Publisher knows we are done
                layout->finishedCount.fetch_add(1, std::memory_order_release);
            }

        } catch (...) {
            // If SHM doesn't exist yet, retry every second
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}