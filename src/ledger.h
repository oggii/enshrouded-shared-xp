// Shared-XP bookkeeping. Pure logic, no game code, so it can be tested on its own.
//
// Sources (the mining and knowledge hooks) record every non-combat gain. The experience
// system visits every player once per tick; on each visit the player receives the gains
// that other players recorded since that player's previous visit.
//
// Joint quests: when players finish the same quest together, the game gives each of them the
// quest XP. With the filter on, a player does not also receive another player's quest gain if
// the player recorded a quest gain of the same amount within kJointWindowMs of it. Quest gains
// wait kQuestHoldMs before they are shared, so the receiver's own gain has time to arrive.
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
    static constexpr uint64_t kQuestHoldMs = 5'000;
    static constexpr uint64_t kJointWindowMs = 10'000;

    // On: quest XP that a receiver earned itself at the same time is not shared to it again.
    void set_skip_joint_quests(bool on);

    // A gain of `amount` XP by the player whose experience component is `player`.
    // Combat and None are ignored. Returns true if the gain was recorded.
    bool record(uintptr_t player, uint32_t amount, uint8_t type, uint64_t now_ms);

    // XP owed to `player` for gains other players recorded since its previous visit,
    // already multiplied. A player seen for the first time gets nothing for the past.
    // `include_own` (test mode) also returns the player's own gains, so one player can
    // check the whole path alone. `skipped` (optional) receives the quest XP left out because
    // the player finished the same quest (before the multiplier).
    uint32_t collect(uintptr_t player, double multiplier, uint64_t now_ms, bool include_own = false,
                     uint32_t* skipped = nullptr);

    // Drops gains and players not seen for `max_age_ms`.
    void prune(uint64_t now_ms, uint64_t max_age_ms);

    size_t pending_gains() const;
    size_t known_players() const;

private:
    struct Gain {
        uint64_t seq;
        uintptr_t player;
        uint32_t amount;
        uint8_t type;
        uint64_t at_ms;
    };
    struct Visit {
        uint64_t seq;
        uint64_t at_ms;
    };

    bool earned_same_quest(uintptr_t player, const Gain& other) const;

    mutable std::mutex mutex_;
    bool skip_joint_quests_ = false;
    uint64_t seq_ = 0;
    std::vector<Gain> gains_;
    std::unordered_map<uintptr_t, Visit> visits_;
};

}  // namespace xpshare
