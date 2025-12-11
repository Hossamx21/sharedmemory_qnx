#pragma once
#include <cstdint>
#include <atomic>

struct RegionHeader {
    uint32_t magic;
    uint32_t capacity;
    uint32_t chunkSize;
    uint32_t headerSize;
    uint32_t payloadOffset;

    // Pulse-based notification fields
    //int32_t notify_pid;      
    /*PID of the subscriber process (consumer).
    The process that wants to receive a QNX pulse when new data arrives. */

    //int32_t notify_chid;     
    /*Channel ID created by consumer using ChannelCreate().
      Pulses are delivered to this channel.*/
      // --- Add for latest-frame mode ---
    /*std::atomic<uint64_t> latest_seq{0};
    std::atomic<uint64_t> latest_ts_us{0};
    std::atomic<uint32_t> latest_idx{0};*/

    // --- Add subscriber table ---
    static constexpr uint32_t MAX_SUBSCRIBERS = 2;

    struct SubscriberInfo {
        std::atomic<int32_t> pid{0};
        std::atomic<int32_t> chid{0};
        std::atomic<bool>    active{false};
    };

    SubscriberInfo subscribers[MAX_SUBSCRIBERS];

};
