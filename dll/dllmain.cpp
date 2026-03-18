#include "init.h"
#include <windows.h>

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID) {
    if (dwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hMod);
        char dll_path[MAX_PATH];
        GetModuleFileNameA(hMod, dll_path, MAX_PATH);
        InitModule(hMod, dll_path);
    }
    return TRUE;
}
