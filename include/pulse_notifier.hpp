#pragma once

#if !defined(__QNX__)
# error "PulseNotifier requires QNX"
#endif
#include <sys/neutrino.h>
#include <sys/netmgr.h>
#include <sys/dispatch.h>
#include <unistd.h>
#include <errno.h>

#include <cstdint>
#include "region_header.hpp"

class PulseNotifier {
public:
    PulseNotifier();
    ~PulseNotifier();

    // Subscriber side: create channel and write pid/chid into RegionHeader
    bool createReceiver(RegionHeader* hdr) noexcept;

    // Publisher side: attach to receiver stored inside RegionHeader
    bool attachToReceiver(const RegionHeader* hdr) noexcept;

    // Multi Subscriber Mode
    bool attachToSpecific(int32_t pid, int32_t chid);


    // Send pulse to receiver; will retry attach automatically
    bool notify(const RegionHeader* hdr) noexcept;

    // Receiver blocks here for pulse
    bool wait() noexcept;

    int getChid() const noexcept { return chid_; }

    // Cleanup
    void close() noexcept;

private:
    int chid_ = -1;   // channel id (subscriber)
    int coid_ = -1;   // connection id (publisher)
};
