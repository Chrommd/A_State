#include "console.h"
#include "init.h"
#include "lua_interface.h"
#include "hooks.h"
#include "logger.h"
#include "MinHook.h"
#include <windows.h>
#include <iostream>
#include <cstdio>
#include <string>

static void PrintHelp() {
    printf("\n");
    printf("==========================================\n");
    printf("         A_State - Command Reference\n");
    printf("==========================================\n");
    printf(" GENERAL\n");
    printf("  help                  This list\n");
    printf("  quit                  Eject A_State cleanly\n");
    printf("  dump                  Dump Lua globals -> A_State_Globals.txt\n");
    printf("  load <file.lua>       Execute a .lua file from path\n");
    printf("  print <expr>          Evaluate and print a Lua expression\n");
    printf("  hook call             Trace Lua function calls\n");
    printf("  hook native           Report native Lua load/runtime errors\n");
    printf("  hook stop             Stop tracing\n");
    printf("  hook cooldown <ms>    Set aggregation window for repeated call entries (default 1000)\n");
    printf("\n");
    printf(" DEBUG UI - OPEN\n");
    printf("  open test\n");
    printf("  open server\n");
    printf("  open ranch\n");
    printf("  open myhorse\n");
    printf("  open injury\n");
    printf("  open injurystatus\n");
    printf("  open ai\n");
    printf("  open skill\n");
    printf("  open var\n");
    printf("  open ghost\n");
    printf("  open room\n");
    printf("\n");
    printf(" DEBUG UI - CLOSE\n");
    printf("  close <name>\n");
    printf("  close all\n");
    printf("\n");
    printf(" UTILITY\n");
    printf("  pos\n");
    printf("  chat <msg>\n");
    printf("  switchhook <0|1>      0=Safe Hook, 1=Direct Pointer\n");
    printf("  (anything else)       Execute as raw Lua code\n");
    printf("==========================================\n\n");
}

static bool IsMultilineStart(const std::string& line) {
    if (line.empty()) return false;
    std::string trimmed = line;
    size_t start = trimmed.find_first_not_of(" \t");
    if (start != std::string::npos) trimmed = trimmed.substr(start);
    
    return (trimmed.find("function") == 0 ||
            trimmed.find("local function") == 0 ||
            trimmed.find("if ") == 0 ||
            trimmed.find("for ") == 0 ||
            trimmed.find("while ") == 0 ||
            line.back() == '=' ||
            line.back() == ',');
}

bool ConsoleRun() {
    printf("\n");
    printf("[A_State] Successfully injected!\n");
    printf("[A_State] Type 'help' for useful commands, or write valid Lua to execute it right away.\n");
    printf("[A_State] For multiline: start typing, then press Enter on empty line to execute.\n");
    printf("\n");

    std::string line;
    std::string accumulated;
    bool should_eject = false;
    while (true) {
        if (accumulated.empty()) {
            printf("A_State> ");
        } else {
            printf("      >> ");
        }
        fflush(stdout);
        if (!std::getline(std::cin, line)) break;
        
        if (!accumulated.empty()) {
            if (line.empty()) {
                line = accumulated;
                accumulated.clear();
            } else {
                accumulated += "\n" + line;
                continue;
            }
        } else if (line.empty()) {
            continue;
        } else if (IsMultilineStart(line)) {
            accumulated = line;
            continue;
        }

        if (line == "help") {
            PrintHelp();

        } else if (line == "quit") {
            Log("[A_State] Ejecting cleanly...");
            HooksShutdown();
            should_eject = true;
            break;

        } else if (line == "log start" || line == "log stop") {
            Log("[A_State] 'log' commands removed. Use 'hook call' to trace Lua.");

        } else if (line == "dump") {
            char exe_path[MAX_PATH];
            GetModuleFileNameA(NULL, exe_path, MAX_PATH);
            std::string exeDir = std::string(exe_path);
            exeDir = exeDir.substr(0, exeDir.find_last_of("\\/"));
            std::string dumpPath = exeDir + "\\A_State_Globals.txt";
            std::string safeDump = dumpPath;
            for (char& c : safeDump) if (c == '\\') c = '/';

            RequestLuaExecute(
                "local f=io.open('" + safeDump + "','w') "
                "if f then for k,v in pairs(_G) do f:write(k..' ('..type(v)..')\\n') end "
                "f:close() return 'Globals dumped!' else return 'Failed to open file.' end"
            );
            Log("[A_State] Globals dumped -> " + dumpPath);

        } else if (line.substr(0, 5) == "load ") {
            std::string file = line.substr(5);
            std::string safe_file = file;
            for (char& c : safe_file) if (c == '\\') c = '/';
            RequestLuaExecute(
                "local s=io.open('" + safe_file + "','r') "
                "if s then local c=s:read('*a') s:close() "
                "local f,err=loadstring(c,'@" + safe_file + "') "
                "if f then local ok,res=pcall(f) if not ok then return '[load error] '..tostring(res) else return res end "
                "else return '[load buffer error] '..tostring(err) end "
                "else return '[A_State] File not found: " + safe_file + "' end"
            );
            Log("[A_State] Loading: " + file);

        } else if (line.substr(0, 6) == "print ") {
            RequestLuaExecute("return (" + line.substr(6) + ")");

        } else if (line == "pos") {
            RequestLuaExecute(
                "local p=util:GetMyPos() "
                "if p and p.x and p.y and p.z then "
                "  return 'Pos: '..tostring(p.x)..', '..tostring(p.y)..', '..tostring(p.z) "
                "else "
                "  return 'Pos: '..tostring(p) "
                "end"
            );

        } else if (line.substr(0, 5) == "chat ") {
            RequestLuaExecute(
                "if util then util:InsertNoticeMsg('" + line.substr(5) + "') return 'Chat sent.' "
                "else return '[A_State] util not available here.' end"
            );

        } else if (line.substr(0, 5) == "hook ") {
            std::string type = line.substr(5);
            if (type == "call") {
                trace_hooks_active = true;
                current_hook_mask  = 1;
                Log("[A_State] Call Hook active.");
            } else if (type == "native") {
                native_error_hooks_active = true;
                Log("[A_State] Native Lua error hook active.");
            } else if (type == "stop") {
                trace_hooks_active = false;
                native_error_hooks_active = false;
                current_hook_mask  = 0;
                Log("[A_State] Hooks stopped.");
            } else if (type.substr(0, 9) == "cooldown ") {
                try {
                    int ms = std::stoi(type.substr(9));
                    hook_cooldown_ms = ms;
                    Log("[A_State] Hook cooldown set to " + std::to_string(ms) + "ms.");
                } catch (...) {
                    Log("[A_State] Use: hook cooldown <ms>  e.g. hook cooldown 2000");
                }
            } else {
                Log("[A_State] Use: hook call | hook native | hook stop");
            }

        } else if (line.substr(0, 11) == "switchhook ") {
            std::string mode_str = line.substr(11);
            if (mode_str == "0") {
                LuaSetExecutionMode(0);
                Log("[A_State] Switched to Mode 0: Safe Hook.");
            } else if (mode_str == "1") {
                LuaSetExecutionMode(1);
                Log("[A_State] Switched to Mode 1: Direct Pointer.");
            } else {
                Log("[A_State] Use: switchhook 0 or switchhook 1");
            }

        } else if (line == "close all") {
            RequestLuaExecute(CloseAllUI());
            Log("[A_State] Closing all debug panels.");

        } else if (line.substr(0, 5) == "open ") {
            std::string key = line.substr(5);
            std::string code = OpenUI(key);
            if (code.empty()) printf("[A_State] Unknown UI '%s'. Type 'help'.\n", key.c_str());
            else { RequestLuaExecute(code); Log("[A_State] Opened: " + key); }

        } else if (line.substr(0, 6) == "close ") {
            std::string key = line.substr(6);
            std::string code = CloseUI(key);
            if (code.empty()) printf("[A_State] Unknown UI '%s'. Type 'help'.\n", key.c_str());
            else { RequestLuaExecute(code); Log("[A_State] Closed: " + key); }

        } else {
            RequestLuaExecute(line);
        }
    }

    return should_eject;
}
