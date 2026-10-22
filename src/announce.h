// Chat line on join: right after the server's own "Player X joined." line, the chat shows one
// line from the mod, "Shared XP ... is active" or, on a server build the mod does not support,
// "Shared XP ... is NOT active". Uses the server's own chat queue and the joining player as
// the sender (the client drops lines without a valid sender); the line shows without a name.
#pragma once

#include <cstdint>
#include <string>

namespace xpshare {

// Finds the join line in the running server and hooks it. Returns false, after logging why,
// when it is not found; XP sharing is not affected either way.
bool install_announce(const uint8_t* base, const std::string& line);

}  // namespace xpshare
