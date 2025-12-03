#include "shm_region.hpp"

ShmRegion::ShmRegion(const std::string& name, std::size_t size, Mode mode)
    : name_(name), size_(size)
{
    creator_ = (mode == Mode::Create);

    if (creator_) {
        fd_ = shm_open(name_.c_str(), O_CREAT | O_RDWR, 0666);
    } else {
        fd_ = shm_open(name_.c_str(), O_RDWR, 0666);
    }

    if (fd_ == -1)
        throw std::runtime_error("Failed to open shared memory");

    if (creator_) {
        if (ftruncate(fd_, static_cast<off_t>(size_)) == -1) //if size_ is larger than off_t max, the cast truncates or is implementation-defined — consider checking overflow.
            throw std::runtime_error("Failed to set shared memory size");
    }

    addr_ = mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
    if (addr_ == MAP_FAILED)
        throw std::runtime_error("Failed to mmap shared memory");
}

ShmRegion::~ShmRegion() {
    if (addr_) munmap(addr_, size_); //should be compared to MAP_FAILED as well addr_ != nullptr && addr_ != MAP_FAILED
    if (fd_ != -1) close(fd_);
    if (creator_) shm_unlink(name_.c_str()); //If other processes still have it open, unlinking only removes the name; the memory remains until all references are gone.
}