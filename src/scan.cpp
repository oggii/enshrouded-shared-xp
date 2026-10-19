#include "scan.h"

#include <windows.h>

#include <cstdio>
#include <cstring>
#include <iterator>
#include <span>

namespace xpshare {

namespace {

// enshrouded_server.exe of Steam build 25988414 (Game Version SVN 1269579).
constexpr DWORD kTimeDateStamp = 1792148062;
constexpr DWORD kSizeOfImage = 0x1dcf000;

struct Pattern {
    const char* name;
    std::span<const uint8_t> bytes;
    size_t hook_offset;
};

constexpr uint8_t kMining[] = {0x0f, 0xb7, 0x4e, 0x04, 0x4c, 0x8b, 0xc3, 0x48, 0x03, 0xce,
                               0x44, 0x01, 0x3c, 0x91, 0x48, 0x8d, 0x4c, 0x24, 0x30};
constexpr uint8_t kKnowledge[] = {0x41, 0x0f, 0xb7, 0x4e, 0x04, 0x4c, 0x8b, 0xc3, 0x49,
                                  0x03, 0xce, 0x01, 0x3c, 0x91, 0x48, 0x8d, 0x4d, 0xf7};
constexpr uint8_t kCommitRead[] = {0x0f, 0xb7, 0x51, 0x0c, 0x0f, 0xb7, 0x4f, 0x04, 0x48,
                                   0x03, 0xcf, 0x40, 0x32, 0xf6, 0x44, 0x8b, 0x3c, 0x91};

// Entry of FUN_1401950f0 up to its first call; the "set" twin FUN_1401970f0 differs in the
// call's rel32, so this only matches the "add" helper.
constexpr uint8_t kAddHelper[] = {0x48, 0x89, 0x5c, 0x24, 0x08, 0x48, 0x89, 0x74, 0x24, 0x10, 0x57,
                                  0x48, 0x83, 0xec, 0x30, 0x41, 0x8b, 0xf0, 0x48, 0x8b, 0xfa, 0x4c,
                                  0x8b, 0xc2, 0x48, 0x8b, 0xd9, 0x41, 0x8b, 0xd1, 0x48, 0x8d, 0x4c,
                                  0x24, 0x20, 0xe8, 0x28, 0xf8, 0x7e, 0x00, 0x0f, 0xb6, 0x4c};
// The helper calls in FUN_1401b7530 and FUN_1401b67a0 (offset = the return address).
constexpr uint8_t kKnowledgeSystemCall[] = {0x44, 0x8b, 0x0d, 0xe8, 0x1b, 0xa2, 0x00, 0x44,
                                            0x8b, 0x45, 0x80, 0x48, 0x8b, 0x55, 0xd0, 0xe8,
                                            0x0f, 0xd9, 0xfd, 0xff, 0x48, 0x8b, 0x7d, 0x20};
constexpr uint8_t kGrantSystemCall[] = {0x44, 0x8b, 0x0d, 0x52, 0x29, 0xa2, 0x00, 0x48, 0x8b,
                                        0xcf, 0x44, 0x8b, 0x45, 0xa8, 0x48, 0x8b, 0x55, 0x08,
                                        0xe8, 0x76, 0xe6, 0xfd, 0xff, 0x48, 0x8b, 0x4d, 0x18};

// Returns the single match + offset, 0 if there is none or more than one.
uintptr_t find_unique(const uint8_t* begin, size_t size, const Pattern& p, int& count) {
    count = 0;
    uintptr_t hit = 0;
    const size_t n = p.bytes.size();
    for (size_t i = 0; i + n <= size; ++i) {
        if (begin[i] == p.bytes[0] && std::memcmp(begin + i, p.bytes.data(), n) == 0) {
            if (++count == 1) {
                hit = reinterpret_cast<uintptr_t>(begin + i) + p.hook_offset;
            }
        }
    }
    return count == 1 ? hit : 0;
}

}  // namespace

bool find_targets(const uint8_t* base, Targets& out, std::string& error) {
    auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    auto nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    char buf[160];
    if (nt->FileHeader.TimeDateStamp != kTimeDateStamp ||
        nt->OptionalHeader.SizeOfImage != kSizeOfImage) {
        std::snprintf(buf, sizeof buf,
                      "unknown server build (TimeDateStamp %lu, SizeOfImage 0x%lx), not hooking",
                      nt->FileHeader.TimeDateStamp, nt->OptionalHeader.SizeOfImage);
        error = buf;
        return false;
    }
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

    const Pattern patterns[] = {
        {"mining", kMining, 10},
        {"knowledge", kKnowledge, 11},
        {"commit_read", kCommitRead, 14},
        {"add_helper", kAddHelper, 0},
        {"knowledge_system_call", kKnowledgeSystemCall, 20},
        {"grant_system_call", kGrantSystemCall, 23},
    };
    uintptr_t* slots[] = {&out.mining,     &out.knowledge,            &out.commit_read,
                          &out.add_helper, &out.knowledge_system_ret, &out.grant_system_ret};
    static_assert(std::size(patterns) == std::size(slots));
    for (size_t i = 0; i < std::size(patterns); ++i) {
        int count = 0;
        *slots[i] = find_unique(code, size, patterns[i], count);
        if (*slots[i] == 0) {
            std::snprintf(buf, sizeof buf, "pattern '%s' matched %d times (need exactly 1), not hooking",
                          patterns[i].name, count);
            error = buf;
            return false;
        }
    }
    return true;
}

}  // namespace xpshare
