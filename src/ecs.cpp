#include "ecs.h"

#include "memory.h"

namespace xpshare {

namespace {
uint32_t g_entity_table = 0;  // 0 = unknown, nothing is read
}  // namespace

void set_entity_table_offset(uint32_t offset) {
    g_entity_table = offset;
}

uint32_t param_entity(uintptr_t param) {
    uintptr_t world = 0, table = 0;
    uint32_t index = 0, id = 0;
    if (g_entity_table == 0 || !read_ptr(param, world) || !read_u32(param + 8, index) || index == 0 ||
        !read_ptr(world + g_entity_table, table) ||
        !read_u32(table + static_cast<uintptr_t>(index - 1) * 16, id)) {
        return 0;
    }
    return id;
}

uint32_t system_entity(uintptr_t param_slot) {
    uintptr_t param = 0;
    return read_ptr(param_slot, param) ? param_entity(param) : 0;
}

}  // namespace xpshare
