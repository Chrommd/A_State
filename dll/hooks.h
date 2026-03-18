#pragma once
#include "addresses.h"
#include <atomic>

extern lua_pcall_t original_lua_pcall;
extern luaL_loadbuffer_t original_luaL_loadbuffer;
extern std::atomic<bool> trace_hooks_active;
extern std::atomic<bool> native_error_hooks_active;
extern std::atomic<int>  current_hook_mask;
extern std::atomic<int>  hook_cooldown_ms;

void HooksInit();
void HooksShutdown();
