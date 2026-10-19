#pragma once

#include <cstdint>
#include <string>

namespace xpshare {

struct Targets {
    uintptr_t mining = 0;       // add [rcx+rdx*4], r15d   (rsi = component, r15d = amount)
    uintptr_t knowledge = 0;    // add [rcx+rdx*4], edi    (r14 = component, edi = amount, byte [r15] = type)
    uintptr_t commit_read = 0;  // mov r15d, [rcx+rdx*4]   (rdi = component, rcx+rdx*4 = pending)
    // FUN_1401950f0(component, ctx, amount, fieldHash): "add to field" helper, hooked at entry.
    uintptr_t add_helper = 0;
    // Return addresses of the two helper calls that add pending XP (not hooked, compared):
    uintptr_t knowledge_system_ret = 0;  // FUN_1401b7530, source type at caller [rsp+0x78]
    uintptr_t grant_system_ret = 0;      // FUN_1401b67a0, source type at caller [rbp-0x60]
};

// Checks that the image mapped at `base` (the running server, or the selftest's mapping of an
// exe on disk) is the analysed build and finds each hook site by a pattern that must match
// exactly once. On failure `error` says why and nothing should be hooked.
bool find_targets(const uint8_t* base, Targets& out, std::string& error);

}  // namespace xpshare
