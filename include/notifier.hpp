#pragma once
#include <mutex>
#include <condition_variable>
#include <atomic>

class Notifier {
public:
    void notify();
    void wait();

private:
    std::mutex mtx_;
    std::condition_variable cv_;
    std::atomic_bool flag_{false};
};
