#include "join_watch.h"

#include <algorithm>

namespace xpshare {

void JoinWatch::update(const std::array<SlotState, kPlayerSlots>& slots, uint64_t now_ms) {
    for (int i = 0; i < kPlayerSlots; ++i) {
        const SlotState& s = slots[i];
        uint32_t entity = s.handle != 0 ? s.entity : 0;
        if (primed_ && entity != 0 && entity != entities_[i]) {
            pending_.push_back({s.handle, s.account, now_ms + delay_ms_});
        }
        entities_[i] = entity;
    }
    primed_ = true;
}

std::vector<Join> JoinWatch::take_due(const std::array<SlotState, kPlayerSlots>& slots, uint64_t now_ms) {
    std::vector<Join> due;
    auto still_here = [&](uint32_t handle) {
        return std::any_of(slots.begin(), slots.end(),
                           [&](const SlotState& s) { return s.handle == handle && s.entity != 0; });
    };
    auto it = pending_.begin();
    while (it != pending_.end()) {
        if (it->due_ms > now_ms) {
            ++it;
            continue;
        }
        if (still_here(it->handle)) due.push_back(*it);
        it = pending_.erase(it);
    }
    return due;
}

}  // namespace xpshare
