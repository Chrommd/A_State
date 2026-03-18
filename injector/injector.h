#pragma once
#include <windows.h>
#include <string>

bool InjectDLL(DWORD processID, const std::string& dllPath);
