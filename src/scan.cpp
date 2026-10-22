#include "scan.h"

#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <vector>

namespace xpshare {

namespace {

// Builds of enshrouded_server.exe this mod was checked against. An unknown build is refused,
// even if every pattern below still matched: the registers at a hook site are only known for
// the builds somebody looked at.
struct Build {
    DWORD time_date_stamp;
    DWORD size_of_image;
    const char* label;
};
constexpr Build kBuilds[] = {
    {1792148062, 0x1dcf000, "25988414"},  // 16 Oct 2026, Game Version SVN 1269579
    {1792507668, 0x1dd1000, "26083076"},  // 20 Oct 2026
};

// Patterns are hex bytes; "??" matches anything (used for call targets and rip-relative
// displacements, which move between builds while the surrounding code stays the same).
struct Pattern {
    const char* name;
    const char* text;
    size_t hook_offset;
};

constexpr const char* kMining = "0f b7 4e 04 4c 8b c3 48 03 ce 44 01 3c 91 48 8d 4c 24 30";
constexpr const char* kKnowledge = "41 0f b7 4e 04 4c 8b c3 49 03 ce 01 3c 91 48 8d 4d f7";
constexpr const char* kCommitRead = "0f b7 51 0c 0f b7 4f 04 48 03 cf 40 32 f6 44 8b 3c 91";

// Entry of the "add to field" helper (FUN_1401950f0 in 25988414) up to its first call. Its
// "set" twin has the same bytes apart from the call target, so this pattern matches twice;
// the right one is the function the two system calls below call.
constexpr const char* kAddHelper =
    "48 89 5c 24 08 48 89 74 24 10 57 48 83 ec 30 41 8b f0 48 8b fa 4c 8b c2 48 8b d9 41 8b d1 48 8d 4c 24 20 "
    "e8 ?? ?? ?? ?? 0f b6 4c";
// The helper calls in the knowledge system (FUN_1401b7530) and the grant system
// (FUN_1401b67a0); the hook offset is the return address.
constexpr const char* kKnowledgeSystemCall =
    "44 8b 0d ?? ?? ?? ?? 44 8b 45 80 48 8b 55 d0 e8 ?? ?? ?? ?? 48 8b 7d 20";
constexpr const char* kGrantSystemCall =
    "44 8b 0d ?? ?? ?? ?? 48 8b cf 44 8b 45 a8 48 8b 55 08 e8 ?? ?? ?? ?? 48 8b 4d 18";

// Combat XP add in FUN_1400ec5c0 and the call after it (FUN_1400a6dd0, the field update).
constexpr const char* kCombat = "47 01 3c 88 e8 ?? ?? ?? ?? 4c 8b b5 80 00 00 00";

// The join/leave chat line in the player table update (FUN_1407b34c0): from
// "cmp qword [rsi+0x60], 0" (chat queue present) up to "mov r8b, 4"; the call of the chat
// function follows at kJoinCallOffset and the next instruction at kJoinAfterOffset. The pattern
// stops before the call because another mod (the live map) may have hooked it already. The loca
// tag of "joined" is the immediate of "mov r9d" at kJoinTagOffset.
constexpr const char* kJoinLine =
    "48 83 7e 60 00 74 ?? 41 8b 4e 0c e8 ?? ?? ?? ?? 49 8b 4e 48 4c 8d 05 ?? ?? ?? ?? 41 8b 16 85 c0 "
    "4c 89 85 ?? ?? ?? ?? 41 b9 ?? ?? ?? ?? 41 b8 ?? ?? ?? ?? c6 85 ?? ?? ?? ?? 00 45 0f 44 c1 "
    "4c 89 a5 ?? ?? ?? ?? 4c 8d 8d ?? ?? ?? ?? 4c 89 4c 24 40 4c 8d 8d ?? ?? ?? ?? 48 89 4c 24 38 "
    "48 8d 8d ?? ?? ?? ?? 89 54 24 30 48 8b 56 60 89 44 24 28 44 89 44 24 20 41 b0 04";
constexpr size_t kJoinTagOffset = 0x29;
constexpr size_t kJoinCallOffset = 0x78;
constexpr size_t kJoinAfterOffset = 0x7d;
// Entry of the chat function FUN_140908ce0(out, chat, type, {text, length}*, locaTag1,
// locaTag2, playerHandle, accountIdHash, {value, has}*).
constexpr const char* kChatPost =
    "40 55 56 41 54 41 55 41 57 48 83 ec 30 49 8b 41 08 33 ed 4d 8b f9 45 0f b6 e0 48 8b f2 4c 8b e9 "
    "48 3d 00 04 00 00 76 16";

struct Parsed {
    std::vector<uint8_t> bytes;
    std::vector<bool> fixed;  // false = wildcard
};

Parsed parse(const char* text) {
    Parsed p;
    for (const char* s = text; *s != 0;) {
        if (*s == ' ') { ++s; continue; }
        if (s[0] == '?' && s[1] == '?') {
            p.bytes.push_back(0);
            p.fixed.push_back(false);
        } else {
            char hex[3] = {s[0], s[1], 0};
            p.bytes.push_back(static_cast<uint8_t>(std::strtoul(hex, nullptr, 16)));
            p.fixed.push_back(true);
        }
        s += 2;
    }
    return p;
}

bool matches_at(const uint8_t* at, const Parsed& p) {
    for (size_t k = 0; k < p.bytes.size(); ++k) {
        if (p.fixed[k] && at[k] != p.bytes[k]) return false;
    }
    return true;
}

// Every match of the pattern inside [begin, begin + size), as addresses of the match start.
std::vector<uintptr_t> find_all(const uint8_t* begin, size_t size, const Parsed& p) {
    std::vector<uintptr_t> hits;
    const size_t n = p.bytes.size();
    if (n == 0 || n > size) return hits;
    for (size_t i = 0; i + n <= size; ++i) {
        if (matches_at(begin + i, p)) hits.push_back(reinterpret_cast<uintptr_t>(begin + i));
    }
    return hits;
}

// The target of the near call (e8 rel32) at `call`.
uintptr_t call_target(uintptr_t call) {
    int32_t rel = 0;
    std::memcpy(&rel, reinterpret_cast<const void*>(call + 1), sizeof rel);
    return call + 5 + static_cast<intptr_t>(rel);
}

}  // namespace

bool find_targets(const uint8_t* base, Targets& out, std::string& error) {
    auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    auto nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    char buf[160];
    const Build* build = nullptr;
    for (const Build& b : kBuilds) {
        if (nt->FileHeader.TimeDateStamp == b.time_date_stamp && nt->OptionalHeader.SizeOfImage == b.size_of_image) {
            build = &b;
        }
    }
    if (build == nullptr) {
        std::snprintf(buf, sizeof buf,
                      "unknown server build (TimeDateStamp %lu, SizeOfImage 0x%lx), not hooking",
                      nt->FileHeader.TimeDateStamp, nt->OptionalHeader.SizeOfImage);
        error = buf;
        return false;
    }
    out.build = build->label;
    const IMAGE_SECTION_HEADER* text = nullptr;
    auto sec = IMAGE_FIRST_SECTION(nt);
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        if (std::memcmp(sec[i].Name, ".text", 5) == 0) {
            text = &sec[i];
        }
    }
    if (text == nullptr) {
        error = "no .text section";
        return false;
    }
    const uint8_t* code = base + text->VirtualAddress;
    const size_t size = text->Misc.VirtualSize;

    // Sites that must match exactly once.
    const Pattern patterns[] = {
        {"mining", kMining, 10},
        {"knowledge", kKnowledge, 11},
        {"commit_read", kCommitRead, 14},
        {"knowledge_system_call", kKnowledgeSystemCall, 20},
        {"grant_system_call", kGrantSystemCall, 23},
        {"combat", kCombat, 0},
    };
    uintptr_t* slots[] = {&out.mining, &out.knowledge, &out.commit_read, &out.knowledge_system_ret,
                          &out.grant_system_ret, &out.combat};
    static_assert(std::size(patterns) == std::size(slots));
    for (size_t i = 0; i < std::size(patterns); ++i) {
        std::vector<uintptr_t> hits = find_all(code, size, parse(patterns[i].text));
        if (hits.size() != 1) {
            std::snprintf(buf, sizeof buf, "pattern '%s' matched %zu times (need exactly 1), not hooking",
                          patterns[i].name, hits.size());
            error = buf;
            return false;
        }
        *slots[i] = hits[0] + patterns[i].hook_offset;
    }
    // The add helper is whatever both system calls call (the call is the 5 bytes before each
    // return address), and it has to look like the helper's entry.
    uintptr_t helper = call_target(out.knowledge_system_ret - 5);
    if (helper != call_target(out.grant_system_ret - 5)) {
        error = "the knowledge and grant systems call different helpers, not hooking";
        return false;
    }
    if (helper < reinterpret_cast<uintptr_t>(code) || helper + 64 > reinterpret_cast<uintptr_t>(code) + size ||
        !matches_at(reinterpret_cast<const uint8_t*>(helper), parse(kAddHelper))) {
        error = "the add helper does not look like the analysed one, not hooking";
        return false;
    }
    out.add_helper = helper;
    return true;
}

bool known_build(const uint8_t* base) {
    auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    auto nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    for (const Build& b : kBuilds) {
        if (nt->FileHeader.TimeDateStamp == b.time_date_stamp && nt->OptionalHeader.SizeOfImage == b.size_of_image) {
            return true;
        }
    }
    return false;
}

bool find_chat_sites(const uint8_t* base, ChatSites& out) {
    auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    auto nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    auto sec = IMAGE_FIRST_SECTION(nt);
    const uint8_t* code = nullptr;
    size_t size = 0;
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        if (std::memcmp(sec[i].Name, ".text", 5) == 0) {
            code = base + sec[i].VirtualAddress;
            size = sec[i].Misc.VirtualSize;
        }
    }
    if (code == nullptr) return false;
    std::vector<uintptr_t> hits = find_all(code, size, parse(kJoinLine));
    std::vector<uintptr_t> chats = find_all(code, size, parse(kChatPost));
    if (hits.size() != 1 || chats.size() != 1) return false;
    const uintptr_t at = hits[0], chat = chats[0];
    // e8 = the call itself (must go to the chat function); e9 = another mod hooked it already.
    const uint8_t call_op = *reinterpret_cast<const uint8_t*>(at + kJoinCallOffset);
    const uint8_t after_op = *reinterpret_cast<const uint8_t*>(at + kJoinAfterOffset);
    if (call_op == 0xe8 ? call_target(at + kJoinCallOffset) != chat : call_op != 0xe9) return false;
    if (after_op != 0x44 && after_op != 0xe9) return false;
    out.join_call = at + kJoinCallOffset;
    out.join_after = at + kJoinAfterOffset;
    out.chat_post = chat;
    std::memcpy(&out.join_tag, reinterpret_cast<const void*>(at + kJoinTagOffset), sizeof out.join_tag);
    return true;
}

}  // namespace xpshare
