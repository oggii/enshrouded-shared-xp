#include "config.h"

#include <windows.h>

#include <cstdlib>

namespace xpshare {

namespace {

// package/xpshare.ini ships the same text; keep both in sync.
constexpr const char* kDefaults =
    "; Shared XP for Enshrouded dedicated servers.\n"
    "; Shares non-combat XP (exploration, books, quests, mining, item sets) with every\n"
    "; other player who is online, at any distance. Combat XP is left to the game.\n"
    "; Changes take effect after a server restart.\n"
    "[xpshare]\n"
    "Enabled=1\n"
    "; Shared XP = the other player's gain x Multiplier (1.0 = the same amount, 0 to 10).\n"
    "Multiplier=1.0\n"
    "; Quests finished together: 0 = each player gets the quest XP once (from the game),\n"
    "; 1 = each player also gets the other players' share, so double quest XP.\n"
    "; With 0, shared quest XP arrives about 5 seconds after the quest.\n"
    "DoubleQuestXp=0\n"
    "Log=1\n"
    "; 1 = one log line per gain and per share (to check that it works), 0 = start-up and errors only.\n"
    "LogEveryShare=0\n"
    "; Test only: 1 = your own non-combat XP is credited to you once more (solo check).\n"
    "TestSelfShare=0\n"
    "; Diagnostics: 1 = log every XP amount a player receives, from any source.\n"
    "LogAllXp=0\n";

}  // namespace

Config load_config(const std::wstring& dir) {
    std::wstring path = dir + L"\\xpshare.ini";
    if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) {
        HANDLE f = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                               FILE_ATTRIBUTE_NORMAL, nullptr);
        if (f != INVALID_HANDLE_VALUE) {
            DWORD written = 0;
            WriteFile(f, kDefaults, static_cast<DWORD>(strlen(kDefaults)), &written, nullptr);
            CloseHandle(f);
        }
    }
    Config c;
    c.enabled = GetPrivateProfileIntW(L"xpshare", L"Enabled", 1, path.c_str()) != 0;
    c.log = GetPrivateProfileIntW(L"xpshare", L"Log", 1, path.c_str()) != 0;
    c.log_every_share = GetPrivateProfileIntW(L"xpshare", L"LogEveryShare", 0, path.c_str()) != 0;
    c.test_self_share = GetPrivateProfileIntW(L"xpshare", L"TestSelfShare", 0, path.c_str()) != 0;
    c.log_all_xp = GetPrivateProfileIntW(L"xpshare", L"LogAllXp", 0, path.c_str()) != 0;
    c.double_quest_xp = GetPrivateProfileIntW(L"xpshare", L"DoubleQuestXp", 0, path.c_str()) != 0;
    wchar_t buf[32];
    GetPrivateProfileStringW(L"xpshare", L"Multiplier", L"1.0", buf, 32, path.c_str());
    double m = wcstod(buf, nullptr);
    c.multiplier = (m >= 0.0 && m <= 10.0) ? m : 1.0;
    return c;
}

}  // namespace xpshare
