// shm_region.hpp
#pragma once
#include <string>
#include <cstddef>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>


class ShmRegion {
public:
    ShmRegion(const std::string& name, std::size_t size);
    ~ShmRegion();

    void* get() noexcept { return addr_; }
    std::size_t getSize() const noexcept { return size_; }

private:
    std::string name_;
    std::size_t size_{};
    int fd_{-1};
    void* addr_{nullptr};
};
