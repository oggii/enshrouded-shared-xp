// Chat announcement: when a player joins, the server's chat shows one system line saying that
// the mod is active. Uses the server's own chat queue, like its "Player X joined." line.
#pragma once

#include "config.h"
#include "scan.h"

namespace xpshare {

// Hooks the player table (optional sites from find_targets). Returns false, after logging
// why, when the sites were not found; XP sharing works either way.
bool install_announce(const Targets& targets, const Config& config, const char* version);

}  // namespace xpshare
