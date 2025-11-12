#include "shm_region.hpp"
#include "publisher.hpp"
#include "subscriber.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>

int main() {
    const char* shmName = "/my_shm";
    const size_t shmSize = 1024;       // 1 KB shared memory
    const size_t chunkSize = 128;      // 128 bytes per chunk

    // Create shared memory region
    ShmRegion region(shmName, shmSize);

    // Create publisher and subscriber
    Publisher pub(region, chunkSize);
    Subscriber sub(region, chunkSize);

    // Data to publish
    const char message[] = "Hello from publisher!";

    // Publisher thread
    std::thread publisherThread([&]() {
        std::cout << "Publishing message..." << std::endl;
        pub.publish(message, sizeof(message));
    });

    // Give some time for publisher to write
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Subscriber thread
    std::thread subscriberThread([&]() {
        void* chunk = region.get(); // For demo, just get base
        std::cout << "Subscriber read: " << static_cast<char*>(chunk) << std::endl;
    });

    publisherThread.join();
    subscriberThread.join();

    return 0;
}
