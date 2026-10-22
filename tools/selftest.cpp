// Offline check: maps an enshrouded_server.exe from disk (without running it) and runs the same
// build check and pattern search the DLL runs inside the server.
//
// Usage: xpshare_selftest <path\to\enshrouded_server.exe>
// Exit code 0 = the mod would hook this build, 1 = it would stay inactive, 2 = usage/load error.
#include <windows.h>

#include <cstdio>
#include <cstring>
#include <string>

#include "scan.h"

namespace {

struct Site {
    const char* name;
    uintptr_t xpshare::Targets::*field;
};

constexpr Site kSites[] = {
    {"commit_read", &xpshare::Targets::commit_read},
    {"mining", &xpshare::Targets::mining},
    {"knowledge", &xpshare::Targets::knowledge},
    {"add_helper", &xpshare::Targets::add_helper},
    {"knowledge_system_ret", &xpshare::Targets::knowledge_system_ret},
    {"grant_system_ret", &xpshare::Targets::grant_system_ret},
    {"combat", &xpshare::Targets::combat},
    {"player_table", &xpshare::Targets::player_table},
    {"chat_post", &xpshare::Targets::chat_post},
};

// Where each site sits in the builds that were analysed (RVAs, same order as kSites).
struct Known {
    const char* build;
    uintptr_t rva[9];
};
constexpr Known kKnown[] = {
    {"25988414", {0xcf018, 0x226cbc, 0x1b566a, 0x1950f0, 0x1b77e1, 0x1b6a7a, 0xecbce, 0x7b3508, 0x908ce0}},
    {"26083076", {0xcfec8, 0x227b6c, 0x1b651a, 0x195fa0, 0x1b8691, 0x1b792a, 0xeda7e, 0x7b5848, 0x90b330}},
};

}  // namespace

int wmain(int argc, wchar_t** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: xpshare_selftest <path\\to\\enshrouded_server.exe>\n");
        return 2;
    }
    HMODULE image = LoadLibraryExW(argv[1], nullptr, DONT_RESOLVE_DLL_REFERENCES);
    if (image == nullptr) {
        std::fprintf(stderr, "could not map %ls (error %lu)\n", argv[1], GetLastError());
        return 2;
    }
    auto base = reinterpret_cast<const uint8_t*>(image);
    xpshare::Targets targets;
    std::string error;
    if (!xpshare::find_targets(base, targets, error)) {
        std::printf("INACTIVE: %s\n", error.c_str());
        return 1;
    }
    const Known* known = nullptr;
    for (const Known& k : kKnown) {
        if (std::strcmp(k.build, targets.build) == 0) known = &k;
    }
    std::printf("server build %s\n", targets.build);
    bool ok = true;
    for (size_t i = 0; i < std::size(kSites); ++i) {
        uintptr_t rva = targets.*kSites[i].field - reinterpret_cast<uintptr_t>(base);
        bool match = known == nullptr || rva == known->rva[i];
        ok = ok && match;
        std::printf("%-21s rva 0x%llx%s\n", kSites[i].name, static_cast<unsigned long long>(rva),
                    match ? "" : "  (UNEXPECTED)");
    }
    std::printf(ok ? "OK: all hook sites found\n" : "FAIL: a hook site moved\n");
    return ok ? 0 : 1;
}
