#include "shm_region.hpp"
#include "pulse_notifier.hpp"
#include "shm_layout.hpp"
#include "publisher.hpp"
#include <iostream>
#include <cstring>
#include <thread>
#include <vector>
#include <iomanip>
#include <sched.h>
#include <sys/neutrino.h>

void notifyAll(RegionHeader* hdr, std::vector<int>& coids) {
    const int PULSE_CODE = 1; 

    for (int i = 0; i < 2; ++i) { // MAX_SUBSCRIBERS = 2
        auto& s = hdr->subscribers[i];
        
        // Skip inactive slots
        if (!s.active.load(std::memory_order_acquire)) {
            // Clean up old connections
            if (coids[i] != -1) {
                ConnectDetach(coids[i]);
                coids[i] = -1;
            }
            continue;
        }

        // Connect if we haven't already
        if (coids[i] == -1) {
            int pid = s.pid.load();
            int chid = s.chid.load();
            if (pid != 0 && chid != 0) {
                coids[i] = ConnectAttach(ND_LOCAL_NODE, pid, chid, _NTO_SIDE_CHANNEL, 0);
            }
        }

        // Send Pulse
        if (coids[i] != -1) {
            MsgSendPulse(coids[i], SIGEV_PULSE_PRIO_INHERIT, PULSE_CODE, 0);
        }
    }
}
// Helper to print Hex
void printHex(const char* label, void* ptr, int count) {
    uint8_t* d = static_cast<uint8_t*>(ptr);
    std::cout << label << " [ ";
    for(int i=0; i<count; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)d[i] << " ";
    }
    std::cout << std::dec << "]\n";
}

int main() {
    const std::string shmName = "/demo_sync_shm";
    // Total Size: Header + (100MB * 2)
    std::size_t totalSize = sizeof(shm_layout) + (FRAME_SIZE * 2);

    // Unlink previous instance to ensure clean start
    shm_unlink(shmName.c_str());

    std::cout << "[PUBLISHER] Allocating " << totalSize / (1024*1024) << " MB...\n";
    ShmRegion region(shmName, totalSize, ShmRegion::Mode::Create);
    
    uint8_t* base = static_cast<uint8_t*>(region.getBase());
    shm_layout* layout = reinterpret_cast<shm_layout*>(base);
    
    // Calculate buffer offsets
    uint8_t* buffer0 = base + sizeof(shm_layout);
    uint8_t* buffer1 = buffer0 + FRAME_SIZE;

    // Initialize Header logic
    if (region.isCreator()) {
        new (&layout->header) RegionHeader();
        layout->currentBufferIdx = 0;
        layout->finishedCount = 2; // Start as "everyone done" so we can write immediately
    }

    // Cache for Connection IDs (One for each subscriber slot)
    std::vector<int> coids(2, -1);

    PulseNotifier notifier;

    // Wait for exactly 2 subscribers (Speed + Steering)
    std::cout << "[PUBLISHER] Waiting for 2 subscribers...\n";
    while (true) {
        int active = 0;
        for(int i=0; i<2; ++i) if(layout->header.subscribers[i].active) active++;
        if(active >= 2) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "[PUBLISHER] Starting 50 FPS Control Loop.\n";

    // Create a local source buffer to simulate camera input
    // We fill 1KB for demonstration, but you can increase this to FRAME_SIZE
    std::vector<uint8_t> sourceData(1024); 

    int frameCount = 0;
    int activeBuf = 0;
    auto nextWakeup = std::chrono::steady_clock::now();

    while (true) {
        // 1. SYNCHRONIZATION: Block until BOTH subscribers finish previous frame
        while (layout->finishedCount.load(std::memory_order_acquire) < 2) {
            sched_yield(); 
        }

        // 2. Double Buffering Logic
        activeBuf = (activeBuf + 1) % 2; 
        uint8_t* targetBuf = (activeBuf == 0) ? buffer0 : buffer1;

        // 3. Prepare Data Pattern
        // Write Frame ID into first 4 bytes
        uint32_t* srcHeader = reinterpret_cast<uint32_t*>(sourceData.data());
        srcHeader[0] = frameCount;

        // Fill remaining bytes with a rolling pattern based on Frame ID
        for(int k=4; k<1024; ++k) {
            sourceData[k] = (uint8_t)((k + frameCount) % 255);
        }

        // 4. MEMCPY (The actual data write)
        std::memcpy(targetBuf, sourceData.data(), 1024); 

        // 5. Commit & Notify
        layout->finishedCount.store(0, std::memory_order_release);
        layout->currentBufferIdx.store(activeBuf, std::memory_order_release);
         notifyAll(&layout->header, coids);

        // 6. 50 FPS Timing Governance
        nextWakeup += std::chrono::milliseconds(20);
        std::this_thread::sleep_until(nextWakeup);

        // Print status once per second (every 50 frames)
        /*if (frameCount % 50 == 0) {
            
        }*/
        std::cout << "[PUB] Wrote Frame " << frameCount << " to Buf " << activeBuf << " ";
        printHex("Data:", targetBuf, 16);
        frameCount++;
    }
    return 0;
}