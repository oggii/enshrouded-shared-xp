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
    // Combat system FUN_1400ec5c0, per receiver of a kill: add [r8+r9*4], r15d
    // (r15d = amount, edi = receiver entity id, r12 = index in the kill's receiver list).
    uintptr_t combat = 0;
    const char* build = nullptr;  // label of the recognised server build, e.g. "26083076"
};

// Checks that the image mapped at `base` (the running server, or the selftest's mapping of an
// exe on disk) is the analysed build and finds each hook site by a pattern that must match
// exactly once. On failure `error` says why and nothing should be hooked.
bool find_targets(const uint8_t* base, Targets& out, std::string& error);

// True if the image is one of the builds the XP sites were checked against.
bool known_build(const uint8_t* base);

// The server's own "Player X joined." chat line, sent from the player table update
// (FUN_1407b34c0). The mod sends its line right after it, to the same chat queue and with the
// same sender. Found by its own pattern on ANY build (the call must go to a function that
// looks like the chat function), so a server build the mod does not support can still say
// so in chat. Nothing here depends on the XP sites.
struct ChatSites {
    uintptr_t join_call = 0;   // call chat_post: rdx = chat queue, [rsp+0x20] = loca tag,
                               // [rsp+0x30] = player handle, [rsp+0x38] = account id hash
    uintptr_t join_after = 0;  // the instruction after that call
    uintptr_t chat_post = 0;   // FUN_140908ce0, queues a chat line for every client
    uint32_t join_tag = 0;     // loca tag of "Player X joined." (the same call sends "left")
};
bool find_chat_sites(const uint8_t* base, ChatSites& out);

}  // namespace xpshare
