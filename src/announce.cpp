#include "announce.h"

#include <windows.h>

#include <array>
#include <cstdio>
#include <string>

#include <safetyhook.hpp>

#include "join_watch.h"
#include "log.h"
#include "memory.h"

namespace xpshare {

namespace {

// Player table (FUN_1407b34c0 in 25988414; at the hook site rsi = the player manager):
// slots at +0x1e0, 0x148 bytes each; +0x0 player handle, +0xA state (3 = in game),
// +0x18 entity id, +0x48 account id hash. The chat queue is at manager +0x60 (0 when the
// server has text chat switched off).
constexpr uintptr_t kSlots = 0x1e0;
constexpr uintptr_t kStride = 0x148;
constexpr uintptr_t kHandle = 0x0;
constexpr uintptr_t kState = 0xA;
constexpr uintptr_t kEntity = 0x18;
constexpr uintptr_t kAccount = 0x48;
constexpr uint8_t kInGame = 3;
constexpr uintptr_t kChat = 0x60;

// FUN_140908ce0(out, chat, type, {text, length}*, locaTag1, locaTag2, playerHandle,
// accountIdHash, {value, has}*): queues a chat line for every connected client. Type 4 with
// no loca tag is shown as plain text without a name, but only with a valid player handle
// (checked in game on build 26083076, 22 Oct 2026).
using ChatPostFn = void* (*)(void* out, uintptr_t chat, uint8_t type, const void* text, uint32_t loca1,
                             uint32_t loca2, uint32_t player, uint64_t account, const void* opt);
constexpr uint8_t kSystemLine = 4;
constexpr uint64_t kDelayMs = 8000;  // the client is still loading when the slot turns "in game"
constexpr uint64_t kPollMs = 250;

SafetyHookMid g_player_table;
ChatPostFn g_post = nullptr;
JoinWatch g_watch(kDelayMs);
uint64_t g_last_poll_ms = 0;
bool g_logged_no_chat = false;
std::string g_line;

bool read_u64(uintptr_t address, uint64_t& out) {
    uintptr_t v = 0;
    if (!read_ptr(address, v)) return false;
    out = v;
    return true;
}

std::array<SlotState, kPlayerSlots> snapshot(uintptr_t manager) {
    std::array<SlotState, kPlayerSlots> slots{};
    for (int i = 0; i < kPlayerSlots; ++i) {
        uintptr_t slot = manager + kSlots + static_cast<uintptr_t>(i) * kStride;
        SlotState s;
        uint8_t state = 0;
        if (!read_u32(slot + kHandle, s.handle) || s.handle == 0) continue;
        if (read_u8(slot + kState, state) && state == kInGame) read_u32(slot + kEntity, s.entity);
        read_u64(slot + kAccount, s.account);
        slots[i] = s;
    }
    return slots;
}

// Runs on the server thread that updates the player table, which also posts the game's own
// join lines, so the chat queue is used from the thread that owns it.
void on_player_table(SafetyHookContext& ctx) {
    uint64_t now = GetTickCount64();
    if (now - g_last_poll_ms < kPollMs) return;
    g_last_poll_ms = now;
    uintptr_t manager = ctx.rsi;
    auto slots = snapshot(manager);
    g_watch.update(slots, now);
    if (g_watch.pending() == 0) return;
    uintptr_t chat = 0;
    if (!read_ptr(manager + kChat, chat) || chat == 0) {
        if (!g_logged_no_chat) {
            g_logged_no_chat = true;
            log("announce: text chat is off on this server, no chat line");
        }
        g_watch.take_due(slots, now);
        return;
    }
    for (const Join& j : g_watch.take_due(slots, now)) {
        struct { const char* ptr; uint64_t length; } text{g_line.c_str(), g_line.size()};
        struct { uint64_t value; uint8_t has; uint8_t pad[7]; } opt{};
        alignas(8) uint8_t out[16] = {};
        g_post(out, chat, kSystemLine, &text, 0, 0, j.handle, j.account, &opt);
        log("announce: chat line for player handle %u%s", j.handle, out[0] == 0 ? "" : " (queue full, not sent)");
    }
}

}  // namespace

bool install_announce(const Targets& t, const Config& config, const char* version) {
    if (t.player_table == 0 || t.chat_post == 0) {
        log("announce: %s not found, no chat line (XP sharing is not affected)",
            t.player_table == 0 ? "player table" : "chat function");
        return false;
    }
    char line[160];
    std::snprintf(line, sizeof line, "Shared XP %s is active: %s XP is shared with everyone online.", version,
                  config.share_combat_xp ? "all" : "non-combat");
    g_line = line;
    g_post = reinterpret_cast<ChatPostFn>(t.chat_post);
    auto hook = safetyhook::MidHook::create(reinterpret_cast<void*>(t.player_table), on_player_table);
    if (!hook) {
        log("announce: could not hook the player table, no chat line");
        return false;
    }
    g_player_table = std::move(*hook);
    log("hooked %-11s at %llx (chat line when a player joins)", "players", static_cast<unsigned long long>(t.player_table));
    return true;
}

}  // namespace xpshare
