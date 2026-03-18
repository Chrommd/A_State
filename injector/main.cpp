#include <windows.h>
#include <iostream>
#include <string>
#include "process_utils.h"
#include "injector.h"

int main() {
    std::cout << "==========================================\n";
    std::cout << "             A_State Injector\n";
    std::cout << "        A_State Lua Engine by Chrommd\n";
    std::cout << "==========================================\n";

    std::cout << "Select Injection Mode:\n";
    std::cout << "1. Auto-Inject (Inject as soon as Alicia is found)\n";
    std::cout << "2. Delayed Inject (Wait X seconds after Alicia is found)\n";
    std::cout << "3. Manual Inject (Wait for you to press Enter)\n";
    std::cout << "Choice (1-3) [Default 1]: ";

    int choice = 1;
    std::string input;
    std::getline(std::cin, input);
    if (!input.empty()) {
        try { choice = std::stoi(input); } catch (...) { choice = 1; }
    }

    int delaySeconds = 0;
    if (choice == 2) {
        std::cout << "Delay in seconds (e.g. 5): ";
        std::getline(std::cin, input);
        if (!input.empty()) {
            try { delaySeconds = std::stoi(input); } catch (...) { delaySeconds = 5; }
        }
    }

    std::cout << "\n[A_State] Waiting for Alicia to launch...\n";

    DWORD processID = 0;
    std::wstring foundName;
    while (!processID) {
        processID = GetProcessIdByName(L"Alicia-tag10.exe");
        if (processID) { foundName = L"Alicia-tag10.exe"; break; }
        processID = GetProcessIdByName(L"Alicia.exe");
        if (processID) { foundName = L"Alicia.exe"; break; }
        Sleep(1000);
    }

    std::wcout << L"[A_State] Found " << foundName << L" PID: " << processID << L"\n";

    if (choice == 2 && delaySeconds > 0) {
        std::cout << "[A_State] Delaying injection for " << delaySeconds << " seconds...\n";
        Sleep(delaySeconds * 1000);
    } else if (choice == 3) {
        std::cout << "[A_State] Press ENTER to inject A_State now...\n";
        std::getline(std::cin, input);
    }

    char exe_path[MAX_PATH];
    GetModuleFileNameA(NULL, exe_path, MAX_PATH);
    std::string exeDir = std::string(exe_path);
    exeDir = exeDir.substr(0, exeDir.find_last_of("\\/"));
    std::string dllPath = exeDir + "\\A_State.dll";

    DWORD attr = GetFileAttributesA(dllPath.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        std::cout << "[A_State] ERROR: DLL not found at: " << dllPath << "\n";
        std::cout << "[A_State] Make sure A_State_Injector.exe is in the same folder as A_State.dll\n";
        system("pause");
        return 1;
    }

    std::cout << "[A_State] Injecting: " << dllPath << "\n";

    if (InjectDLL(processID, dllPath)) {
        std::cout << "[A_State] Success! A_State Lua Engine is running and ready.\n";
    } else {
        std::cout << "[A_State] Injection failed!\n";
    }

    std::cout << "\nYou can safely close this window.\n";
    system("pause");
    return 0;
}
