#pragma once
#include <windows.h>
#include <string>

DWORD GetProcessIdByName(const std::wstring& processName);
