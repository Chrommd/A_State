#pragma once
#include <windows.h>

typedef unsigned int lua_State;

#define LUA_GLOBALSINDEX -10002

#define LUA_PCALL        0x009c8aa0
#define LUAL_LOADBUFFER  0x009c9c70
#define LUA_TONUMBER     0x009c8050
#define LUA_TOINTEGER    0x009c8090
#define LUA_TOLSTRING    0x009c8100
#define LUA_TOBOOLEAN    0x009c80d0
#define LUA_TYPE         0x009c7ea0
#define LUA_TYPENAME     0x009c7ec0
#define LUA_SETTOP       0x009c7cb0
#define LUA_GETTOP       0x009c7c90
#define LUA_PUSHVALUE    0x009c7e70
#define LUA_PUSHNUMBER   0x009c82f0
#define LUA_PUSHSTRING   0x009c8380
#define LUA_PUSHCCLOSURE 0x009c8450
#define LUA_RAWGETI      0x009c8630
#define LUA_GETFIELD     0x009c8590
#define LUA_RAWGET       0x009c85f0
#define LUA_NEWTABLE     0x009c8670
#define LUA_GETMETATABLE 0x009c86c0
#define LUA_SETTABLE     0x009c8790
#define LUA_SETFIELD     0x009c87c0
#define LUA_RAWSET       0x009c8820
#define LUA_RAWSETI      0x009c8890
#define LUA_SETMETATABLE 0x009c8900
#define LUA_SETFENV      0x009c89b0
#define LUA_SETHOOK      0x009cd130
#define LUA_GETSTACK     0x009cd190
#define LUA_GETINFO      0x009cde90
#define LUA_NEXT         0x009c8cc0
#define LUA_NEWUSERDATA  0x009c8d80

#define LUA_STATE_PTR    0x00d3a8e4

struct lua_Debug {
    int event;
    const char* name;
    const char* namewhat;
    const char* what;
    const char* source;
    int currentline;
    int nups;
    int linedefined;
    char short_src[60];
    int i_ci;
};

typedef void(__cdecl*  lua_Hook)          (lua_State* L, lua_Debug* ar);
typedef int(__cdecl*   lua_sethook_t)     (lua_State* L, lua_Hook func, int mask, int count);
typedef int(__cdecl*   lua_getinfo_t)     (lua_State* L, const char* what, lua_Debug* ar);
typedef int(__cdecl*   luaL_loadbuffer_t) (lua_State* L, const char* buff, int sz, const char* name);
typedef int(__cdecl*   lua_pcall_t)       (lua_State* L, int nargs, int nresults, int errfunc);
typedef int(__cdecl*   lua_gettop_t)      (lua_State* L);
typedef void(__cdecl*  lua_settop_t)      (lua_State* L, int idx);
typedef int(__cdecl*   lua_type_t)        (lua_State* L, int idx);
typedef const char*(__cdecl* lua_typename_t)(lua_State* L, int tp);
typedef int(__cdecl*   lua_toboolean_t)   (lua_State* L, int idx);
typedef const char*(__cdecl* lua_tolstring_t)(lua_State* L, int idx, size_t* len);
typedef double(__cdecl* lua_tonumber_t)   (lua_State* L, int idx);
typedef int(__cdecl*   lua_tointeger_t)   (lua_State* L, int idx);
typedef void(__cdecl*  lua_pushvalue_t)   (lua_State* L, int idx);
typedef void(__cdecl*  lua_pushnumber_t)  (lua_State* L, double n);
typedef void(__cdecl*  lua_pushstring_t)  (lua_State* L, const char* s);
typedef void(__cdecl*  lua_pushcclosure_t)(lua_State* L, void* fn, int n);
typedef void(__cdecl*  lua_newtable_t)    (lua_State* L);
typedef void*(__cdecl* lua_newuserdata_t) (lua_State* L, unsigned int size);
typedef void(__cdecl*  lua_rawgeti_t)     (lua_State* L, int idx, int n);
typedef void(__cdecl*  lua_getfield_t)    (lua_State* L, int idx, const char* k);
typedef void(__cdecl*  lua_rawget_t)      (lua_State* L, int idx);
typedef int(__cdecl*   lua_getmetatable_t)(lua_State* L, int objindex);
typedef void(__cdecl*  lua_settable_t)    (lua_State* L, int idx);
typedef void(__cdecl*  lua_setfield_t)    (lua_State* L, int idx, const char* k);
typedef void(__cdecl*  lua_rawset_t)      (lua_State* L, int idx);
typedef void(__cdecl*  lua_rawseti_t)     (lua_State* L, int idx, int n);
typedef int(__cdecl*   lua_setmetatable_t)(lua_State* L, int objindex);
typedef int(__cdecl*   lua_setfenv_t)     (lua_State* L, int idx);
typedef int(__cdecl*   lua_getstack_t)    (lua_State* L, int level, lua_Debug* ar);
typedef int(__cdecl*   lua_next_t)        (lua_State* L, int idx);

inline void lua_getglobal(lua_State* L, const char* name) {
    ((lua_getfield_t)LUA_GETFIELD)(L, LUA_GLOBALSINDEX, name);
}

inline void lua_setglobal(lua_State* L, const char* name) {
    ((lua_setfield_t)LUA_SETFIELD)(L, LUA_GLOBALSINDEX, name);
}
