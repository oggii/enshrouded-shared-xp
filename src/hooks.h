#pragma once

#include <string>

#include "config.h"
#include "scan.h"

namespace xpshare {

// Installs the three mid-function hooks. Returns false and fills `error` if any fails;
// in that case none stays installed.
bool install_hooks(const Targets& targets, const Config& config, std::string& error);

}  // namespace xpshare
