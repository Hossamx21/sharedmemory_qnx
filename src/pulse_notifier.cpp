#include "pulse_notifier.hpp"

#include <sys/neutrino.h>
#include <sys/netmgr.h>
#include <sys/dispatch.h>
#include <unistd.h>
#include <errno.h>

static constexpr int PULSE_CODE_DATA = 1;

PulseNotifier::PulseNotifier() = default;
PulseNotifier::~PulseNotifier() { close(); }

bool PulseNotifier::createReceiver(RegionHeader* hdr) noexcept {
    if (!hdr) return false;
    if (chid_ != -1) return true;

    chid_ = ChannelCreate(0);
    if (chid_ == -1) return false;

    hdr->notify_pid  = getpid();
    hdr->notify_chid = chid_;
    return true;
}

bool PulseNotifier::attachToReceiver(const RegionHeader* hdr) noexcept {
    if (!hdr) return false;

    if (coid_ != -1)
        return true;

    int32_t pid  = hdr->notify_pid;
    int32_t chid = hdr->notify_chid;

    if (pid == 0 || chid == 0)
        return false;

    coid_ = ConnectAttach(ND_LOCAL_NODE, pid, chid, _NTO_SIDE_CHANNEL, 0);
    if (coid_ == -1) {
        coid_ = -1;
        return false;
    }
    return true;
}

bool PulseNotifier::notify(const RegionHeader* hdr) noexcept {
    if (!hdr) return false;

    if (coid_ == -1) {
        if (!attachToReceiver(hdr))
            return false;
    }

    int rc = MsgSendPulse(coid_, SIGEV_PULSE_PRIO_INHERIT, PULSE_CODE_DATA, 0);
    if (rc == -1) {
        ConnectDetach(coid_);
        coid_ = -1;
        return false;
    }
    return true;
}

bool PulseNotifier::wait() noexcept {
    if (chid_ == -1) return false;

    struct _pulse pulse;
    ssize_t n = MsgReceive(chid_, &pulse, sizeof(pulse), nullptr);
    if (n == -1) return false;

    return true; // pulse received
}

void PulseNotifier::close() noexcept {
    if (coid_ != -1) {
        ConnectDetach(coid_);
        coid_ = -1;
    }
    if (chid_ != -1) {
        ChannelDestroy(chid_);
        chid_ = -1;
    }
}

/*bool PulseNotifier::attachWithRetry(const RegionHeader* hdr, int attempts, int msDelay) noexcept {
    for (int i = 0; i < attempts; ++i) {
        if (attachToReceiver(hdr)) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(msDelay));
    }
    return false;
}*/
