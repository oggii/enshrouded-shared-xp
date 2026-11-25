#pragma once

#include <cstdint>

namespace xpshare {

// Offset of the entity table in the ECS world, as the game's own entity helper reads it
// (0x828 up to build 26083076, 0x830 in 26430253). Set once before the hooks go in.
void set_entity_table_offset(uint32_t offset);

// Entity id that an ECS system is visiting right now, read like the game's FUN_140653a80:
// the system's first parameter points to {world, index}; id = [[world+table] + (index-1)*16].
// `param` is that parameter itself (e.g. still in rbx). 0 if unreadable.
uint32_t param_entity(uintptr_t param);

// Same, for a system that saved its parameter in memory: `param_slot` is where (e.g. [rbp+0x130]).
uint32_t system_entity(uintptr_t param_slot);

}  // namespace xpshare
