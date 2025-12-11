#pragma once
#include "region_header.hpp"
#include <atomic>
#include <cstddef>
// 100 MB per frame
constexpr std::size_t FRAME_SIZE = 100 * 1024 * 1024;

struct shm_layout {
    RegionHeader header;

    // SYNCHRONIZATION
    // 0 or 1: Which buffer is currently valid for reading?
    std::atomic<int> currentBufferIdx{0};

    // How many subscribers have finished processing the current frame?
    // Publisher waits until this equals 2.
    std::atomic<int> finishedCount{0};

    // THE DATA BUFFERS
    // We define these as offsets or raw pointers in the cpp logic, 
    // but effectively they follow this struct in memory.
};