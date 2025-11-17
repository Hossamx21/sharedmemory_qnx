#include "notifier.hpp"

void Notifier::notify() {
    {
        std::lock_guard<std::mutex> lock(mtx_);
        flag_.store(true, std::memory_order_release);
    }
    cv_.notify_one();
}

void Notifier::wait() {
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [&]() { return flag_.load(std::memory_order_acquire); });
    flag_.store(false, std::memory_order_release);
}
