#include "shm_region.hpp"
#include "publisher.hpp"
#include "subscriber.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

int main() {
    const std::string shmName = "/demo_shm";
    const std::size_t chunkSize = 256;
    const std::size_t totalSize = chunkSize * 8; // 8 chunks

    // Create region
    ShmRegion region(shmName, totalSize);

    Publisher pub(region, chunkSize);
    Subscriber sub(region, chunkSize);

    // Publisher thread
    std::thread pubThread([&]() {
        for (int i = 0; i < 5; ++i) {
            std::string msg = "message " + std::to_string(i);
            pub.publishBlocking(msg.data(), msg.size() + 1);
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
                sub.release(ptr);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }
    });

    pubThread.join();
    subThread.join();

    return 0;
}
