#pragma once

#include <windows.h>

namespace xpshare {

// Loads the real powrprof.dll from the system directory and points every stub at it.
// Runs inside DllMain; returns false if the real library could not be loaded.
bool load_real_powrprof(HMODULE self);

}  // namespace xpshare
