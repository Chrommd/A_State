#pragma once
#include "addresses.h"
#include <string>

void LuaInterfaceInit();
void RequestLuaExecute(const std::string& code);
void LuaRunCode(lua_State* L, const std::string& code);
void CheckAPIs();

bool               LuaGetExecuteFlag();
void               LuaSetExecuteFlag(bool val);
const std::string& LuaGetPendingCode();

void        LuaSetExecutionMode(int mode);
int         LuaGetExecutionMode();

std::string OpenUI(const std::string& key);
std::string CloseUI(const std::string& key);
std::string CloseAllUI();

extern const std::string LOAD_DEBUG_UI;
