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
    enum class Mode {
        Create,
        Attach
    };

    ShmRegion(const std::string& name, std::size_t size, Mode mode);
    ~ShmRegion();

    void* getBase() noexcept { return addr_; }
    std::size_t getSize() const noexcept { return size_; }

    bool isCreator() const noexcept { return creator_; }

private:
    std::string name_;
    std::size_t size_;
    bool creator_ = false;

    int fd_ = -1;
    void* addr_ = nullptr;
};