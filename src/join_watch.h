// Spots players who just came into the game, from snapshots of the server's player table.
// Pure bookkeeping (no game memory), so it can be unit-tested.
#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace xpshare {

constexpr int kPlayerSlots = 16;

struct SlotState {
    uint32_t handle = 0;   // player handle (the chat sender id); 0 = empty slot
    uint32_t entity = 0;   // player entity id while in game, else 0
    uint64_t account = 0;  // account id hash
};

struct Join {
    uint32_t handle = 0;
    uint64_t account = 0;
    uint64_t due_ms = 0;  // when to announce
};

class JoinWatch {
public:
    explicit JoinWatch(uint64_t delay_ms) : delay_ms_(delay_ms) {}

    // Feeds one snapshot of every slot. A slot whose entity went from 0 to non-zero, or to a
    // different entity, is a join; it is announced `delay_ms` later (the client is still
    // loading when the server marks it in game). The very first snapshot only records who is
    // already there, so a restart of the mod does not greet everybody.
    void update(const std::array<SlotState, kPlayerSlots>& slots, uint64_t now_ms);

    // Joins that are due and whose player is still in game (same handle in some slot).
    std::vector<Join> take_due(const std::array<SlotState, kPlayerSlots>& slots, uint64_t now_ms);

    size_t pending() const { return pending_.size(); }

private:
    uint64_t delay_ms_;
    bool primed_ = false;
    std::array<uint32_t, kPlayerSlots> entities_{};
    std::vector<Join> pending_;
};

}  // namespace xpshare
