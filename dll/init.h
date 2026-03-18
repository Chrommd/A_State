#pragma once
#include <windows.h>

extern HMODULE g_hModule;

void InitModule(HMODULE hMod, const char* dll_path);
