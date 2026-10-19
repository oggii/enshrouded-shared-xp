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

bool Ledger::record(uintptr_t player, uint32_t amount, uint8_t type, uint64_t now_ms) {
    if (amount == 0 || type == static_cast<uint8_t>(Source::Combat) ||
        type == static_cast<uint8_t>(Source::None)) {
        return false;
    }
    std::lock_guard lock(mutex_);
    gains_.push_back({++seq_, player, amount, now_ms});
    return true;
}

uint32_t Ledger::collect(uintptr_t player, double multiplier, uint64_t now_ms, bool include_own) {
    std::lock_guard lock(mutex_);
    auto [it, first_visit] = visits_.try_emplace(player, Visit{seq_, now_ms});
    if (first_visit) {
        return 0;
    }
    uint64_t total = 0;
    for (const Gain& gain : gains_) {
        if (gain.seq > it->second.seq && (include_own || gain.player != player)) {
            total += gain.amount;
        }
    }
    it->second = {seq_, now_ms};
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
