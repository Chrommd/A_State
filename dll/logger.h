#pragma once
#include <string>

void LoggerInit(const std::string& log_path);
void LoggerWriteSessionHeader(const std::string& dll_path);
void Log(const std::string& msg);
