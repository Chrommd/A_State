#include "injector.h"
#include <iostream>

bool InjectDLL(DWORD processID, const std::string& dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (!hProcess) {
        std::cerr << "[A_State] Failed to open process. Try running as Administrator.\n";
        return false;
    }
    void* loc = VirtualAllocEx(hProcess, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!loc) {
        std::cerr << "[A_State] Failed to allocate memory in target process.\n";
        CloseHandle(hProcess);
        return false;
    }
    if (!WriteProcessMemory(hProcess, loc, dllPath.c_str(), dllPath.length() + 1, 0)) {
        std::cerr << "[A_State] Failed to write memory in target process.\n";
        VirtualFreeEx(hProcess, loc, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    HANDLE hThread = CreateRemoteThread(hProcess, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, loc, 0, 0);
    if (!hThread) {
        std::cerr << "[A_State] Failed to create remote thread.\n";
        VirtualFreeEx(hProcess, loc, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    WaitForSingleObject(hThread, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);
    VirtualFreeEx(hProcess, loc, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);
    if (exitCode == 0) {
        std::cerr << "[A_State] LoadLibraryA failed inside target process. DLL may be blocked or incompatible.\n";
        return false;
    }
    return true;
}
