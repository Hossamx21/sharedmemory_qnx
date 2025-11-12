// shm_region.cpp
#include "shm_region.hpp"

ShmRegion::ShmRegion(const std::string& name, std::size_t size)
    : name_(name), size_(size)
{
    fd_ = shm_open(name_.c_str(), O_CREAT | O_RDWR, 0666);
    if (fd_ == -1)
        throw std::runtime_error("Failed to open shared memory: " + name_);

    if (ftruncate(fd_, static_cast<off_t>(size_)) == -1)
        throw std::runtime_error("Failed to set shared memory size");

    addr_ = mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
    if (addr_ == MAP_FAILED)
        throw std::runtime_error("Failed to map shared memory");
}

ShmRegion::~ShmRegion() {
    if (addr_) munmap(addr_, size_);
    if (fd_ != -1) close(fd_);
    shm_unlink(name_.c_str());
}
