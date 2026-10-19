// Shared-XP bookkeeping. Pure logic, no game code, so it can be tested on its own.
//
// Sources (the mining and knowledge hooks) record every non-combat gain. The experience
// system visits every player once per tick; on each visit the player receives the gains
// that other players recorded since that player's previous visit.
#pragma once

#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace xpshare {

// ExperienceSourceType of build 25988414 (reflection table at 0x140d83950).
enum class Source : uint8_t {
    None = 0,
    Exploration = 1,
    Lore = 2,
    Quest = 3,
    Combat = 4,
    Mining = 5,
    ItemSetCompleted = 6,
};

const char* source_name(uint8_t type);

class Ledger {
public:
    // A gain of `amount` XP by the player whose experience component is `player`.
    // Combat and None are ignored. Returns true if the gain was recorded.
    bool record(uintptr_t player, uint32_t amount, uint8_t type, uint64_t now_ms);

    // XP owed to `player` for gains other players recorded since its previous visit,
    // already multiplied. A player seen for the first time gets nothing for the past.
    // `include_own` (test mode) also returns the player's own gains, so one player can
    // check the whole path alone.
    uint32_t collect(uintptr_t player, double multiplier, uint64_t now_ms, bool include_own = false);

    // Drops gains and players not seen for `max_age_ms`.
    void prune(uint64_t now_ms, uint64_t max_age_ms);

    size_t pending_gains() const;
    size_t known_players() const;

private:
    struct Gain {
        uint64_t seq;
        uintptr_t player;
        uint32_t amount;
        uint64_t at_ms;
    };
    struct Visit {
        uint64_t seq;
        uint64_t at_ms;
    };

    mutable std::mutex mutex_;
    uint64_t seq_ = 0;
    std::vector<Gain> gains_;
    std::unordered_map<uintptr_t, Visit> visits_;
};

}  // namespace xpshare
