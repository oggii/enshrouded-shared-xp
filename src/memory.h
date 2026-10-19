#pragma once

#include <cstdint>

namespace xpshare {

// Reads game memory under a structured-exception guard: a bad address returns false
// instead of crashing the server.
bool read_u8(uintptr_t address, uint8_t& out);
bool read_u32(uintptr_t address, uint32_t& out);
bool read_ptr(uintptr_t address, uintptr_t& out);
bool write_u32(uintptr_t address, uint32_t value);

}  // namespace xpshare
