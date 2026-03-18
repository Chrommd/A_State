#include "logger.h"
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <windows.h>

static std::string s_log_path;

static void AppendLine(const std::string& msg) {
    std::ofstream ofs(s_log_path, std::ios_base::app);
    if (ofs.is_open()) ofs << msg << "\n";
}

void LoggerInit(const std::string& log_path) {
    s_log_path = log_path;
    std::ofstream ofs(s_log_path, std::ios_base::out);
}

void LoggerWriteSessionHeader(const std::string& dll_path) {
    SYSTEMTIME st{};
    GetLocalTime(&st);

    char exe_path[MAX_PATH]{};
    GetModuleFileNameA(nullptr, exe_path, MAX_PATH);

    char cwd[MAX_PATH]{};
    GetCurrentDirectoryA(MAX_PATH, cwd);

    std::ostringstream header;
    header << "==== A_State Session "
           << std::setfill('0')
           << std::setw(4) << st.wYear << "-"
           << std::setw(2) << st.wMonth << "-"
           << std::setw(2) << st.wDay << " "
           << std::setw(2) << st.wHour << ":"
           << std::setw(2) << st.wMinute << ":"
           << std::setw(2) << st.wSecond
           << " ====\n"
           << "[Session] PID=" << GetCurrentProcessId() << " TID=" << GetCurrentThreadId() << "\n"
           << "[Session] DLL=" << dll_path << "\n"
           << "[Session] EXE=" << exe_path << "\n"
           << "[Session] CWD=" << cwd;

    std::ofstream ofs(s_log_path, std::ios_base::app);
    if (ofs.is_open()) {
        ofs << header.str() << "\n";
    }
}

void Log(const std::string& msg) {
    printf("%s\n", msg.c_str());
    AppendLine(msg);
}
