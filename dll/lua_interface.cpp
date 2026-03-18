#include "lua_interface.h"
#include "hooks.h"
#include "logger.h"
#include <windows.h>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <map>
#include <atomic>
#include <cstdio>

static bool        s_execute_flag  = false;
static std::string s_pending_code;
static int         s_execution_mode = 0;
static std::string s_lua_log_path;

std::atomic<bool> lua_logging_active{false};

bool        LuaGetExecuteFlag()             { return s_execute_flag; }
void        LuaSetExecuteFlag(bool val)     { s_execute_flag = val; }
const std::string& LuaGetPendingCode()      { return s_pending_code; }

void LuaRunCode(lua_State* L, const std::string& code) {
    int saved_top = ((lua_gettop_t)LUA_GETTOP)(L);
    const bool direct_mode = (s_execution_mode == 1);
    luaL_loadbuffer_t loadbuffer = (luaL_loadbuffer_t)LUAL_LOADBUFFER;
    int load_err = loadbuffer(L, code.c_str(), (int)code.size(), "=[A_State]");
    if (load_err != 0) {
        int t = ((lua_type_t)LUA_TYPE)(L, -1);
        if (t == 4) {
            const char* msg = ((lua_tolstring_t)LUA_TOLSTRING)(L, -1, nullptr);
            if (direct_mode) printf("\r[SyntaxError] %s\n", msg ? msg : "?");
            else printf("\r[SyntaxError] %s\nA_State> ", msg ? msg : "?");
        } else {
            if (direct_mode) printf("\r[SyntaxError] (error type %d)\n", t);
            else printf("\r[SyntaxError] (error type %d)\nA_State> ", t);
        }
        fflush(stdout);
        ((lua_settop_t)LUA_SETTOP)(L, saved_top);
        return;
    }
    int call_err = original_lua_pcall(L, 0, -1, 0);
    if (call_err != 0) {
        int t = ((lua_type_t)LUA_TYPE)(L, -1);
        if (t == 4) {
            const char* msg = ((lua_tolstring_t)LUA_TOLSTRING)(L, -1, nullptr);
            if (direct_mode) printf("\r[RuntimeError] %s\n", msg ? msg : "?");
            else printf("\r[RuntimeError] %s\nA_State> ", msg ? msg : "?");
        } else {
            if (direct_mode) printf("\r[RuntimeError] (non-string error, type %d)\n", t);
            else printf("\r[RuntimeError] (non-string error, type %d)\nA_State> ", t);
        }
        fflush(stdout);
    } else {
        int top = ((lua_gettop_t)LUA_GETTOP)(L);
        int n_res = top - saved_top;
        if (n_res > 0) {
            printf("\r[Output] ");
            for (int i = 1; i <= n_res; i++) {
                int idx = saved_top + i;
                int t = ((lua_type_t)LUA_TYPE)(L, idx);
                if (t == 3 || t == 4) { // number or string
                    const char* s = ((lua_tolstring_t)LUA_TOLSTRING)(L, idx, nullptr);
                    printf("%s\t", s ? s : "nil");
                } else if (t == 1) { // boolean
                    int b = ((lua_toboolean_t)LUA_TOBOOLEAN)(L, idx);
                    printf("%s\t", b ? "true" : "false");
                } else { // other
                    const char* name = ((lua_typename_t)LUA_TYPENAME)(L, t);
                    printf("(%s)\t", name ? name : "?");
                }
            }
            if (direct_mode) printf("\n");
            else printf("\nA_State> ");
            fflush(stdout);
        }
    }
    ((lua_settop_t)LUA_SETTOP)(L, saved_top);
}

void LuaInterfaceInit() {
}

void RequestLuaExecute(const std::string& code) {
    if (s_execution_mode == 1) {
        lua_State* L = *(lua_State**)LUA_STATE_PTR;
        if (L) {
            LuaRunCode(L, code);
        } else {
            Log("[A_State] Error: Global Lua State pointer is null right now!");
        }
    } else {
        s_pending_code  = code;
        s_execute_flag  = true;
    }
}

void CheckAPIs() {
    Log("[A_State] DLL Injected successfully.");
    Log("[A_State] Initializing Engine...");
    struct ApiCheck { const char* name; uintptr_t address; };
    std::vector<ApiCheck> apis = {
        {"lua_pcall",        LUA_PCALL},
        {"luaL_loadbuffer",  LUAL_LOADBUFFER},
        {"lua_tonumber",     LUA_TONUMBER},
        {"lua_tointeger",    LUA_TOINTEGER},
        {"lua_tolstring",    LUA_TOLSTRING},
        {"lua_toboolean",    LUA_TOBOOLEAN},
        {"lua_type",         LUA_TYPE},
        {"lua_typename",     LUA_TYPENAME},
        {"lua_settop",       LUA_SETTOP},
        {"lua_gettop",       LUA_GETTOP},
        {"lua_pushvalue",    LUA_PUSHVALUE},
        {"lua_pushnumber",   LUA_PUSHNUMBER},
        {"lua_pushstring",   LUA_PUSHSTRING},
        {"lua_pushcclosure", LUA_PUSHCCLOSURE},
        {"lua_rawgeti",      LUA_RAWGETI},
        {"lua_getfield",     LUA_GETFIELD},
        {"lua_rawget",       LUA_RAWGET},
        {"lua_newtable",     LUA_NEWTABLE},
        {"lua_getmetatable", LUA_GETMETATABLE},
        {"lua_settable",     LUA_SETTABLE},
        {"lua_setfield",     LUA_SETFIELD},
        {"lua_rawset",       LUA_RAWSET},
        {"lua_rawseti",      LUA_RAWSETI},
        {"lua_setmetatable", LUA_SETMETATABLE},
        {"lua_setfenv",      LUA_SETFENV},
        {"lua_sethook",      LUA_SETHOOK},
        {"lua_getstack",     LUA_GETSTACK},
        {"lua_getinfo",      LUA_GETINFO},
        {"lua_next",         LUA_NEXT},
        {"lua_newuserdata",  LUA_NEWUSERDATA},
        {"lua_state_ptr",    LUA_STATE_PTR},
    };
    for (const auto& api : apis) {
        std::stringstream ss;
        ss << "(success) " << api.name << " found at 0x"
           << std::setfill('0') << std::setw(8) << std::hex << api.address;
        Log(ss.str());
    }
    Log("[A_State] Engine Ready!");
}

static int s_execution_mode_getter() { return s_execution_mode; }

void LuaSetExecutionMode(int mode)   { s_execution_mode = mode; }
int  LuaGetExecutionMode()           { return s_execution_mode; }

const std::string LOAD_DEBUG_UI =
    "package.loaded['logichelper']=nil "
    "package.loaded['rfhelper']=nil "
    "require('logichelper') require('rfhelper') "
    "local s=io.open('script_src/logic/logic_debug_ui.lua','r') "
    "if s then loadstring(s:read('*a'))() s:close() end ";

static const std::map<std::string, std::string> UI_NAMES = {
    {"test",         "TestUI"},
    {"server",       "ServerCommandUI"},
    {"ranch",        "RanchHorseUI"},
    {"myhorse",      "MyHorseUI"},
    {"injury",       "InjuryTestUI"},
    {"injurystatus", "InjuryStatusUI"},
    {"ai",           "AISettingUI"},
    {"skill",        "SkillSystemTestUI"},
    {"var",          "VarSystemUI"},
    {"ghost",        "SelectGhostUI"},
    {"room",         "SingleGameRoomUI"},
};

std::string OpenUI(const std::string& key) {
    auto it = UI_NAMES.find(key);
    if (it == UI_NAMES.end()) return "";
    const std::string& ui = it->second;
    return LOAD_DEBUG_UI + ui + ".Create() " + ui + ".Show()";
}

std::string CloseUI(const std::string& key) {
    auto it = UI_NAMES.find(key);
    if (it == UI_NAMES.end()) return "";
    const std::string& ui = it->second;
    return LOAD_DEBUG_UI + "if " + ui + " and " + ui + ".Close then " + ui + ".Close() end";
}

std::string CloseAllUI() {
    std::string code = LOAD_DEBUG_UI;
    for (const auto& pair : UI_NAMES) {
        const std::string& ui = pair.second;
        code += "if " + ui + " and " + ui + ".Close then " + ui + ".Close() end ";
    }
    return code;
}
