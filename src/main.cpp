// xpshare: shares non-combat XP between all online players of an Enshrouded dedicated server.
// Loaded by the server as powrprof.dll (Wine: WINEDLLOVERRIDES="powrprof=n,b").
#include <windows.h>

#include <string>

#include "config.h"
#include "hooks.h"
#include "log.h"
#include "proxy.h"
#include "scan.h"

namespace {

constexpr const char* kVersion = XPSHARE_VERSION;  // from project() in CMakeLists.txt
bool g_real_loaded = false;

std::wstring exe_dir() {
    wchar_t path[MAX_PATH];
    DWORD len = GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring dir(path, len);
    return dir.substr(0, dir.find_last_of(L"\\/"));
}

DWORD WINAPI start(LPVOID) {
    std::wstring dir = exe_dir();
    xpshare::Config config = xpshare::load_config(dir);
    xpshare::log_open(dir, config.log);
    xpshare::log("xpshare %s starting: Enabled=%d Multiplier=%.2f real powrprof %s", kVersion,
                 config.enabled, config.multiplier, g_real_loaded ? "loaded" : "MISSING");
    if (!config.enabled) {
        xpshare::log("disabled in xpshare.ini, server runs without sharing");
        return 0;
    }
    xpshare::Targets targets;
    std::string error;
    auto base = reinterpret_cast<const uint8_t*>(GetModuleHandleW(nullptr));
    if (!xpshare::find_targets(base, targets, error) ||
        !xpshare::install_hooks(targets, config, error)) {
        xpshare::log("%s; server runs without sharing", error.c_str());
        return 0;
    }
    xpshare::log("active: non-combat XP is shared with every online player%s",
                 config.test_self_share ? " (TEST MODE: own XP is credited once more)" : "");
    return 0;
}

}  // namespace

BOOL WINAPI DllMain(HINSTANCE self, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(self);
        g_real_loaded = xpshare::load_real_powrprof(self);
        if (HANDLE thread = CreateThread(nullptr, 0, start, nullptr, 0, nullptr)) {
            CloseHandle(thread);
        }
    }
    return TRUE;
}
