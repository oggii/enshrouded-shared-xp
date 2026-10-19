#include "ecs.h"

#include "memory.h"

namespace xpshare {

uint32_t param_entity(uintptr_t param) {
    uintptr_t world = 0, table = 0;
    uint32_t index = 0, id = 0;
    if (!read_ptr(param, world) || !read_u32(param + 8, index) || index == 0 ||
        !read_ptr(world + 0x828, table) ||
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
