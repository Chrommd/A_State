#include "init.h"
#include "logger.h"
#include "hooks.h"
#include "lua_interface.h"
#include "console.h"
#include <windows.h>
#include <string>

HMODULE g_hModule = nullptr;

static DWORD WINAPI ConsoleThreadEntry(LPVOID) {
    bool should_eject = ConsoleRun();
    if (should_eject) {
        HWND hwnd = GetConsoleWindow();
        FreeConsole();
        if (hwnd) PostMessage(hwnd, WM_CLOSE, 0, 0);
        FreeLibraryAndExitThread(g_hModule, 0);
    }
    return 0;
}

static DWORD WINAPI MainThread(LPVOID param) {
    const char* dll_path = (const char*)param;
    std::string path(dll_path);
    std::string dir = path.substr(0, path.find_last_of("\\/"));

    LoggerInit(dir + "\\A_State_Diagnostics.txt");
    LoggerWriteSessionHeader(path);
    LuaInterfaceInit();

    CheckAPIs();
    HooksInit();

    AllocConsole();
    SetConsoleTitleA("A_State Lua Engine");
    freopen("CONOUT$", "w", stdout);
    freopen("CONIN$",  "r", stdin);

    Log("[A_State] MinHook attached.");
    Beep(500, 200);

    CreateThread(nullptr, 0, ConsoleThreadEntry, nullptr, 0, nullptr);
    return TRUE;
}

void InitModule(HMODULE hMod, const char* dll_path) {
    g_hModule = hMod;
    char* path_copy = new char[MAX_PATH];
    strcpy_s(path_copy, MAX_PATH, dll_path);
    CreateThread(nullptr, 0, MainThread, path_copy, 0, nullptr);
}
