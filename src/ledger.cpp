#include "ledger.h"

#include <algorithm>
#include <cmath>

namespace xpshare {

const char* source_name(uint8_t type) {
    switch (static_cast<Source>(type)) {
        case Source::None: return "None";
        case Source::Exploration: return "Exploration";
        case Source::Lore: return "Lore";
        case Source::Quest: return "Quest";
        case Source::Combat: return "Combat";
        case Source::Mining: return "Mining";
        case Source::ItemSetCompleted: return "ItemSetCompleted";
    }
    return "Unknown";
}

bool Ledger::record(uintptr_t player, uint32_t amount, uint8_t type, uint64_t now_ms, uint64_t kill) {
    if (amount == 0 || type == static_cast<uint8_t>(Source::None)) {
        return false;
    }
    std::lock_guard lock(mutex_);
    if (type == static_cast<uint8_t>(Source::Combat) && (!share_combat_ || kill == 0)) {
        return false;
    }
    gains_.push_back({++seq_, player, amount, type, kill, now_ms});
    return true;
}

void Ledger::set_skip_joint_quests(bool on) {
    std::lock_guard lock(mutex_);
    skip_joint_quests_ = on;
}

void Ledger::set_share_combat(bool on) {
    std::lock_guard lock(mutex_);
    share_combat_ = on;
}

bool Ledger::first_of_kill(const Gain& gain) const {
    for (const Gain& g : gains_) {
        if (g.seq >= gain.seq) return true;  // gains_ is in seq order
        if (g.type == static_cast<uint8_t>(Source::Combat) && g.kill == gain.kill) return false;
    }
    return true;
}

uint32_t Ledger::kill_share(uintptr_t player, const Gain& first, bool include_own) const {
    uint32_t best = 0;
    for (const Gain& g : gains_) {
        if (g.seq < first.seq || g.type != static_cast<uint8_t>(Source::Combat) || g.kill != first.kill) {
            continue;
        }
        if (g.player == player && !include_own) return 0;  // was at the kill, has its own XP
        best = std::max(best, g.amount);
    }
    return best;
}

bool Ledger::earned_same_quest(uintptr_t player, const Gain& other) const {
    for (const Gain& own : gains_) {
        if (own.player == player && own.seq != other.seq &&
            own.type == static_cast<uint8_t>(Source::Quest) && own.amount == other.amount) {
            uint64_t gap = own.at_ms > other.at_ms ? own.at_ms - other.at_ms : other.at_ms - own.at_ms;
            if (gap <= kJointWindowMs) return true;
        }
    }
    return false;
}

uint32_t Ledger::collect(uintptr_t player, double multiplier, uint64_t now_ms, bool include_own,
                         uint32_t* skipped) {
    std::lock_guard lock(mutex_);
    if (skipped != nullptr) *skipped = 0;
    auto [it, first_visit] = visits_.try_emplace(player, Visit{seq_, now_ms});
    if (first_visit) {
        return 0;
    }
    uint64_t total = 0;
    uint64_t done = it->second.seq;
    for (const Gain& gain : gains_) {  // in seq order
        if (gain.seq <= it->second.seq) {
            continue;
        }
        const bool quest = gain.type == static_cast<uint8_t>(Source::Quest);
        const bool combat = gain.type == static_cast<uint8_t>(Source::Combat);
        if ((skip_joint_quests_ && quest && now_ms < gain.at_ms + kQuestHoldMs) ||
            (combat && now_ms < gain.at_ms + kCombatHoldMs)) {
            break;  // too early to tell; this and every later gain wait for the next visit
        }
        done = gain.seq;
        if (combat) {
            // A kill is settled once, at its first gain; its other gains only count there.
            if (first_of_kill(gain)) total += kill_share(player, gain, include_own);
            continue;
        }
        if (!include_own && gain.player == player) {
            continue;
        }
        if (skip_joint_quests_ && quest && earned_same_quest(player, gain)) {
            if (skipped != nullptr) *skipped += gain.amount;
            continue;
        }
        total += gain.amount;
    }
    it->second = {done, now_ms};
    if (total == 0 || multiplier <= 0.0) {
        return 0;
    }
    double owed = std::floor(static_cast<double>(total) * multiplier);
    return static_cast<uint32_t>(std::min(owed, 1'000'000.0));
}

void Ledger::prune(uint64_t now_ms, uint64_t max_age_ms) {
    std::lock_guard lock(mutex_);
    std::erase_if(gains_, [&](const Gain& g) { return now_ms - g.at_ms > max_age_ms; });
    std::erase_if(visits_, [&](const auto& v) { return now_ms - v.second.at_ms > max_age_ms; });
}

size_t Ledger::pending_gains() const {
    std::lock_guard lock(mutex_);
    return gains_.size();
}

size_t Ledger::known_players() const {
    std::lock_guard lock(mutex_);
    return visits_.size();
}

}  // namespace xpshare
