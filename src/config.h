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
};

// Reads xpshare.ini from `dir`; writes it with the defaults if it does not exist.
Config load_config(const std::wstring& dir);

}  // namespace xpshare
