// The hooks. Register and frame layouts are documented in docs/HOW-IT-WORKS.md (build 25988414).
//
// Players are identified by their entity id, the number the game writes into every XP event.
// The experience component's address is not stable: it moves, e.g. on fast travel.
#include "hooks.h"

#include <windows.h>

#include <safetyhook.hpp>

#include "ecs.h"
#include "ledger.h"
#include "log.h"
#include "memory.h"

namespace xpshare {

namespace {

Config g_config;
Ledger g_ledger;
SafetyHookMid g_mining;
SafetyHookMid g_knowledge;
SafetyHookMid g_commit_read;
SafetyHookMid g_add_helper;
Targets g_targets;
uint64_t g_last_prune_ms = 0;

constexpr uint32_t kPendingXpField = 0x1d7dd102;
constexpr uint64_t kMaxAgeMs = 120'000;

void record(const char* hook, uint32_t entity, uint32_t amount, uint8_t type) {
    if (entity == 0) {
        log("SKIP   %s: no entity id for +%u XP (%s)", hook, amount, source_name(type));
        return;
    }
    if (g_ledger.record(entity, amount, type, GetTickCount64()) && g_config.log_every_share) {
        log("GAIN   entity %u +%u XP (%s, %s)", entity, amount, source_name(type), hook);
    }
}

// Reads a 32-bit entity id stored at `address`; 0 if unreadable.
uint32_t entity_at(uintptr_t address) {
    uint32_t id = 0;
    return read_u32(address, id) ? id : 0;
}

// Mining (FUN_140226c40): add [rcx+rdx*4], r15d; r15d = amount, rbp = the function's context,
// entity id at context+0x14c (what the game writes into the XP event).
void on_mining(SafetyHookContext& ctx) {
    record("mining", entity_at(ctx.rbp + 0x14c), static_cast<uint32_t>(ctx.r15),
           static_cast<uint8_t>(Source::Mining));
}

// Knowledge grants (FUN_1401b5490): add [rcx+rdx*4], edi; edi = amount, r15 = the
// {type, amount} entry, entity id = *param_3 with param_3 saved at [rbp+0x6f].
void on_knowledge(SafetyHookContext& ctx) {
    uint8_t type = 0;
    uintptr_t param_3 = 0;
    if (!read_u8(ctx.r15, type) || !read_ptr(ctx.rbp + 0x6f, param_3)) {
        return;
    }
    record("knowledge", entity_at(param_3), static_cast<uint32_t>(ctx.rdi), type);
}

// FUN_1401950f0 entry: r8d = amount, r9d = field hash, [rsp] = return address. Only pending-XP
// adds from the two known systems count; type and entity sit in the caller's frame/registers
// (the helper has not touched rsp, rbp or r15 yet).
void on_add_helper(SafetyHookContext& ctx) {
    if (static_cast<uint32_t>(ctx.r9) != kPendingXpField) {
        return;
    }
    uintptr_t ret = 0;
    if (!read_ptr(ctx.rsp, ret)) {
        return;
    }
    uint8_t type = 0;
    uint32_t entity = 0;
    const char* hook;
    if (ret == g_targets.knowledge_system_ret) {  // FUN_1401b7530: type [rsp+0x78], entity [r15]
        hook = "knowledge_system";
        read_u8(ctx.rsp + 8 + 0x78, type);
        entity = entity_at(ctx.r15);
    } else if (ret == g_targets.grant_system_ret) {  // FUN_1401b67a0: type [rbp-0x60], entity [rbp+0xa0]
        hook = "grant_system";
        read_u8(ctx.rbp - 0x60, type);
        entity = entity_at(ctx.rbp + 0xa0);
    } else {
        static uintptr_t last_unknown = 0;
        if (ret != last_unknown) {
            last_unknown = ret;
            log("SKIP   pending XP +%u from unknown caller %llx, not shared",
                static_cast<uint32_t>(ctx.r8), static_cast<unsigned long long>(ret));
        }
        return;
    }
    record(hook, entity, static_cast<uint32_t>(ctx.r8), type);  // r8d = the helper's amount
}

// Experience system (FUN_1400cedd0), once per player per tick: mov r15d, [rcx+rdx*4] reads
// pending XP. Adding to pending here lets the game commit, level up and sync it.
void on_commit_read(SafetyHookContext& ctx) {
    uint64_t now = GetTickCount64();
    uint32_t entity = system_entity(ctx.rbp + 0x130);  // param_1 saved at [rbp+0x130]
    auto pending = reinterpret_cast<uint32_t*>(ctx.rcx + ctx.rdx * 4);
    if (g_config.log_all_xp && *pending != 0) {
        log("XP     entity %u receives %u XP (all sources, before sharing)", entity, *pending);
    }
    if (entity != 0) {
        uint32_t owed = g_ledger.collect(entity, g_config.multiplier, now, g_config.test_self_share);
        if (owed != 0) {
            uint32_t before = *pending;
            *pending = before + owed;
            if (g_config.log_every_share) {
                log("SHARE  entity %u +%u XP (pending %u -> %u)", entity, owed, before, before + owed);
            }
        }
    }
    if (now - g_last_prune_ms > 30'000) {
        g_last_prune_ms = now;
        g_ledger.prune(now, kMaxAgeMs);
    }
}

bool create(SafetyHookMid& slot, uintptr_t target, safetyhook::MidHookFn fn, const char* name,
            std::string& error) {
    auto hook = safetyhook::MidHook::create(reinterpret_cast<void*>(target), fn);
    if (!hook) {
        error = std::string("could not hook ") + name;
        return false;
    }
    slot = std::move(*hook);
    log("hooked %-11s at %llx", name, static_cast<unsigned long long>(target));
    return true;
}

}  // namespace

bool install_hooks(const Targets& t, const Config& config, std::string& error) {
    g_config = config;
    g_targets = t;
    // The reader first, so the ledger knows every player before the first gain arrives.
    if (create(g_commit_read, t.commit_read, on_commit_read, "commit_read", error) &&
        create(g_mining, t.mining, on_mining, "mining", error) &&
        create(g_knowledge, t.knowledge, on_knowledge, "knowledge", error) &&
        create(g_add_helper, t.add_helper, on_add_helper, "add_helper", error)) {
        return true;
    }
    g_add_helper = {};
    g_knowledge = {};
    g_mining = {};
    g_commit_read = {};
    return false;
}

}  // namespace xpshare
