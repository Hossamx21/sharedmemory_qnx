#include "pulse_notifier.hpp"

static constexpr int PULSE_CODE_DATA = 1;

PulseNotifier::PulseNotifier() = default;
PulseNotifier::~PulseNotifier() { close(); }
bool PulseNotifier::createReceiver(RegionHeader* /*hdr*/) noexcept {
    if (chid_ != -1) return true;

    chid_ = ChannelCreate(0);
    if (chid_ == -1) return false;

    // IMPORTANT: Do NOT write to any global RegionHeader fields here.
    // Registration into the RegionHeader's subscribers[] is done by caller,
    // which will store getChid() / getpid() into the subscriber slot.
    return true;
}

//It writes hdr->notify_pid/notify_chid directly, overwriting previous values. If multiple subscribers run simultaneously and use the same header, they will overwrite one another.

bool PulseNotifier::attachToReceiver(const RegionHeader* hdr) noexcept {
    if (!hdr) return false;

    if (coid_ != -1)
        return true;

    // Use subscriber slot 0 by convention
    const auto& s = hdr->subscribers[0];

    int32_t pid  = s.pid.load();
    int32_t chid = s.chid.load();

    if (!s.active.load() || pid == 0 || chid == 0)
        return false;

    coid_ = ConnectAttach(ND_LOCAL_NODE, pid, chid, _NTO_SIDE_CHANNEL, 0);
    if (coid_ == -1) {
        coid_ = -1;
        return false;
    }
    return true;
}


bool PulseNotifier::attachToSpecific(int32_t pid, int32_t chid) {
    if (pid == 0 || chid == 0) return false;

    // detach any stale connection first
    if (coid_ != -1) {
        ConnectDetach(coid_);
        coid_ = -1;
    }

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

    ssize_t n = MsgReceive(chid_, &pulse, sizeof(pulse), nullptr); //MsgReceivePulse?
    if (n == -1) return false;

    if (pulse.code != PULSE_CODE_DATA) return false;

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
