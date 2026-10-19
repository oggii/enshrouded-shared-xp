// Offline check: maps an enshrouded_server.exe from disk (without running it) and runs the same
// build check and pattern search the DLL runs inside the server.
//
// Usage: xpshare_selftest <path\to\enshrouded_server.exe>
// Exit code 0 = the mod would hook this build, 1 = it would stay inactive, 2 = usage/load error.
#include <windows.h>

#include <cstdio>
#include <string>

#include "scan.h"

namespace {

struct Site {
    const char* name;
    uintptr_t xpshare::Targets::*field;
    uintptr_t expected_rva;  // build 25988414; 0 = not checked
};

constexpr Site kSites[] = {
    {"commit_read", &xpshare::Targets::commit_read, 0xcf018},
    {"mining", &xpshare::Targets::mining, 0x226cbc},
    {"knowledge", &xpshare::Targets::knowledge, 0x1b566a},
    {"add_helper", &xpshare::Targets::add_helper, 0x1950f0},
    {"knowledge_system_ret", &xpshare::Targets::knowledge_system_ret, 0},
    {"grant_system_ret", &xpshare::Targets::grant_system_ret, 0},
    {"combat", &xpshare::Targets::combat, 0xecbce},
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
    bool ok = true;
    for (const Site& site : kSites) {
        uintptr_t rva = targets.*site.field - reinterpret_cast<uintptr_t>(base);
        bool match = site.expected_rva == 0 || rva == site.expected_rva;
        ok = ok && match;
        std::printf("%-21s rva 0x%llx%s\n", site.name, static_cast<unsigned long long>(rva),
                    match ? "" : "  (UNEXPECTED)");
    }
    std::printf(ok ? "OK: all hook sites found\n" : "FAIL: a hook site moved\n");
    return ok ? 0 : 1;
}
