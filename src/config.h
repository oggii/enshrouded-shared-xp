#pragma once

#include <string>

namespace xpshare {

struct Config {
    bool enabled = true;
    double multiplier = 1.0;  // shared XP = gain × multiplier, 0 disables sharing
    bool log = true;
    bool log_every_share = false;
    // Test mode for one player: your own non-combat gains are credited to you once more,
    // so the whole path can be checked without a second player. Not for normal play.
    bool test_self_share = false;
    // Diagnostics: log every pending XP the experience system commits, from any source,
    // to find gains no hook recorded.
    bool log_all_xp = false;
    // 1: players who finish the same quest together also get each other's share (double
    // quest XP). 0: a player does not receive quest XP it just earned itself.
    bool double_quest_xp = false;
    // 1: combat XP is shared too. Players who were not near a kill get the XP the players at
    // the kill got (the largest amount, once per kill); players at the kill get nothing extra.
    bool share_combat_xp = false;
    // 1: one chat line "Shared XP ... is active" when a player joins.
    bool announce_in_chat = true;
    // Test only: behave as on a server build the mod does not support (no XP hooks, the
    // chat line says the mod is not active).
    bool test_unknown_build = false;
};

// Reads xpshare.ini from `dir`; writes it with the defaults if it does not exist.
Config load_config(const std::wstring& dir);

}  // namespace xpshare
