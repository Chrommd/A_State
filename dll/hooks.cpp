#include "hooks.h"
#include "lua_interface.h"
#include "logger.h"
#include "MinHook.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <windows.h>

lua_pcall_t              original_lua_pcall  = nullptr;
luaL_loadbuffer_t        original_luaL_loadbuffer = nullptr;
std::atomic<bool>        trace_hooks_active  {false};
std::atomic<bool>        native_error_hooks_active{false};
std::atomic<int>         current_hook_mask   {0};
std::atomic<int>         hook_cooldown_ms    {1000};

namespace fs = std::filesystem;

static constexpr const char* LUA_TINKER_DOBUFFER_NAME = "lua_tinker::dobuffer()";

struct ScriptFileEntry {
    std::string path;
    fs::path disk_path;
    size_t size = 0;
    std::uint64_t hash = 0;
};

static std::once_flag s_script_index_once;
static std::mutex s_script_index_mutex;
static std::unordered_map<std::uint64_t, std::vector<ScriptFileEntry>> s_script_index;
static std::unordered_map<std::uint64_t, std::string> s_resolved_chunk_names;

static std::uint64_t HashBytes(const char* data, size_t size) {
    const std::uint64_t fnv_offset = 1469598103934665603ull;
    const std::uint64_t fnv_prime = 1099511628211ull;

    std::uint64_t hash = fnv_offset;
    for (size_t i = 0; i < size; ++i) {
        hash ^= static_cast<unsigned char>(data[i]);
        hash *= fnv_prime;
    }
    hash ^= static_cast<std::uint64_t>(size);
    hash *= fnv_prime;
    return hash;
}

static bool ReadWholeFile(const fs::path& path, std::string& out) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;

    file.seekg(0, std::ios::end);
    std::streamoff end = file.tellg();
    if (end < 0) return false;

    out.resize(static_cast<size_t>(end));
    file.seekg(0, std::ios::beg);

    if (!out.empty()) {
        file.read(out.data(), static_cast<std::streamsize>(out.size()));
        if (!file) return false;
    }

    return true;
}

static std::vector<fs::path> GetScriptRoots() {
    std::vector<fs::path> roots;
    std::error_code ec;

    fs::path cwd = fs::current_path(ec);
    if (!ec) {
        roots.push_back(cwd / "script_src");
        roots.push_back(cwd / "script");
    }

    char exe_path[MAX_PATH]{};
    DWORD len = GetModuleFileNameA(nullptr, exe_path, MAX_PATH);
    if (len > 0 && len < MAX_PATH) {
        fs::path exe_dir = fs::path(exe_path).parent_path();
        for (const char* folder_name : {"script_src", "script"}) {
            fs::path exe_root = exe_dir / folder_name;
            bool already_present = false;
            for (const auto& root : roots) {
                if (root == exe_root) {
                    already_present = true;
                    break;
                }
            }
            if (!already_present) {
                roots.push_back(exe_root);
            }
        }
    }

    return roots;
}

static std::string MakeChunkNameFromPath(const fs::path& path) {
    std::error_code ec;
    fs::path relative = fs::relative(path, fs::current_path(ec), ec);
    if (ec) {
        relative = path;
    }
    return "@" + relative.generic_string();
}

static void BuildScriptIndex() {
    std::call_once(s_script_index_once, []() {
        std::unordered_map<std::uint64_t, std::vector<ScriptFileEntry>> local_index;

        for (const fs::path& root : GetScriptRoots()) {
            std::error_code root_ec;
            if (!fs::exists(root, root_ec) || root_ec) {
                continue;
            }

            fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, root_ec);
            fs::recursive_directory_iterator end;
            while (!root_ec && it != end) {
                const fs::directory_entry& entry = *it;
                std::error_code file_ec;
                if (entry.is_regular_file(file_ec) && !file_ec) {
                    const fs::path extension = entry.path().extension();
                    if (extension != ".lua" && extension != ".luc") {
                        it.increment(root_ec);
                        continue;
                    }

                    std::string contents;
                    if (ReadWholeFile(entry.path(), contents)) {
                        ScriptFileEntry script_entry;
                        script_entry.path = MakeChunkNameFromPath(entry.path());
                        script_entry.disk_path = entry.path();
                        script_entry.size = contents.size();
                        script_entry.hash = HashBytes(contents.data(), contents.size());
                        local_index[script_entry.hash].push_back(std::move(script_entry));
                    }
                }
                it.increment(root_ec);
            }
        }

        std::lock_guard<std::mutex> lock(s_script_index_mutex);
        s_script_index = std::move(local_index);
    });
}

static std::string ResolveLuaTinkerChunkName(const char* buff, int sz) {
    if (!buff || sz < 0) return {};

    const size_t size = static_cast<size_t>(sz);
    const std::uint64_t hash = HashBytes(buff, size);

    {
        std::lock_guard<std::mutex> lock(s_script_index_mutex);
        auto cache_it = s_resolved_chunk_names.find(hash);
        if (cache_it != s_resolved_chunk_names.end()) {
            return cache_it->second;
        }
    }

    BuildScriptIndex();

    std::string resolved;
    std::vector<ScriptFileEntry> candidates;

    {
        std::lock_guard<std::mutex> lock(s_script_index_mutex);
        auto it = s_script_index.find(hash);
        if (it != s_script_index.end()) {
            candidates = it->second;
        }
    }

    for (const ScriptFileEntry& entry : candidates) {
        if (entry.size != size) {
            continue;
        }

        std::string file_contents;
        if (!ReadWholeFile(entry.disk_path, file_contents)) {
            continue;
        }

        if (file_contents.size() == size && std::memcmp(file_contents.data(), buff, size) == 0) {
            resolved = entry.path;
            break;
        }
    }

    {
        std::lock_guard<std::mutex> lock(s_script_index_mutex);
        s_resolved_chunk_names.emplace(hash, resolved);
    }

    return resolved;
}

static std::string GetEffectiveChunkName(const char* buff, int sz, const char* name) {
    if (name && std::strcmp(name, LUA_TINKER_DOBUFFER_NAME) == 0) {
        std::string resolved = ResolveLuaTinkerChunkName(buff, sz);
        if (!resolved.empty()) {
            return resolved;
        }
    }

    return (name && name[0]) ? std::string(name) : std::string();
}

static std::string ParseSource(const char* source) {
    if (!source || !source[0]) return "?";
    if (source[0] == '@') return std::string(source + 1);
    if (source[0] == '=') return std::string(source + 1);
    return "[string]";
}

static std::string SafeStr(const char* s, const char* fallback = "?") {
    return (s && s[0]) ? std::string(s) : std::string(fallback);
}

static std::string TruncateForLog(const char* s, size_t max_len = 220) {
    if (!s) return "?";
    std::string out;
    out.reserve(max_len);
    for (size_t i = 0; s[i] && i < max_len; ++i) {
        unsigned char c = (unsigned char)s[i];
        out.push_back((c >= 32 && c <= 126) ? (char)c : '?');
    }
    return out;
}

struct HookAggregateEntry {
    DWORD last_print_tick = 0;
    unsigned int count = 0;
    SYSTEMTIME last_time{};
    DWORD last_rel_ms = 0;
    std::string nameStr;
    std::string nameWhat;
    std::string whatStr;
    std::string src;
    int currentline = 0;
    int linedefined = 0;
};

static void PrintHookAggregate(const HookAggregateEntry& entry) {
    printf("[%02d:%02d:%02d.%03d +%lu ms] [Hook-CALL x%u] %s() cur=%d def=%d what=%s",
           entry.last_time.wHour, entry.last_time.wMinute, entry.last_time.wSecond, entry.last_time.wMilliseconds,
           (unsigned long)entry.last_rel_ms,
           entry.count,
           entry.nameStr.c_str(),
           entry.currentline,
           entry.linedefined,
           entry.whatStr.c_str());

    if (!entry.nameWhat.empty()) {
        printf(" namewhat=%s", entry.nameWhat.c_str());
    }

    printf(" src=[%s]", entry.src.c_str());

    printf("\n");
}

static void __cdecl TracingHook(lua_State* L, lua_Debug* ar) {
    if (!trace_hooks_active.load()) return;
    ((lua_getinfo_t)LUA_GETINFO)(L, "nSl", ar);

    if (ar->what && ar->what[0] == 'C') return;
    if (ar->event != 0) return;

    static DWORD start_tick = GetTickCount();
    DWORD now = GetTickCount();
    DWORD rel_ms = now - start_tick;

    SYSTEMTIME st;
    GetLocalTime(&st);

    const int cooldown = hook_cooldown_ms.load();
    std::string src        = ParseSource(ar->source);
    std::string nameStr    = SafeStr(ar->name, "<anon>");
    std::string nameWhat   = SafeStr(ar->namewhat, "");
    std::string whatStr    = SafeStr(ar->what, "?");
    std::string key        = nameStr + "|" + std::to_string(ar->currentline) + "|" + std::to_string(ar->linedefined) + "|" + src;

    static std::unordered_map<std::string, HookAggregateEntry> aggregates;
    HookAggregateEntry& entry = aggregates[key];
    entry.count += 1;
    entry.last_time = st;
    entry.last_rel_ms = rel_ms;
    entry.nameStr = nameStr;
    entry.nameWhat = nameWhat;
    entry.whatStr = whatStr;
    entry.src = src;
    entry.currentline = ar->currentline;
    entry.linedefined = ar->linedefined;

    if (entry.last_print_tick == 0 || (now - entry.last_print_tick) >= (DWORD)cooldown) {
        PrintHookAggregate(entry);
        entry.count = 0;
        entry.last_print_tick = now;
    }
}

static int __cdecl detoured_lua_pcall(lua_State* L, int nargs, int nresults, int errfunc) {
    if (LuaGetExecuteFlag()) {
        LuaSetExecuteFlag(false);
        LuaRunCode(L, LuaGetPendingCode());
    }

    if (trace_hooks_active.load() && current_hook_mask.load() > 0) {
        ((lua_sethook_t)LUA_SETHOOK)(L, TracingHook, current_hook_mask.load(), 0);
    } else if (!trace_hooks_active.load()) {
        ((lua_sethook_t)LUA_SETHOOK)(L, TracingHook, 0, 0);
    }

    int result = original_lua_pcall(L, nargs, nresults, errfunc);

    if (native_error_hooks_active.load() && result != 0) {
        int top = ((lua_gettop_t)LUA_GETTOP)(L);
        int t = (top > 0) ? ((lua_type_t)LUA_TYPE)(L, -1) : -1;
        if (t == 4) {
            const char* msg = ((lua_tolstring_t)LUA_TOLSTRING)(L, -1, nullptr);
            printf("[Hook-NATIVE-PCALL-ERROR] code=%d msg=%s\n", result, TruncateForLog(msg).c_str());
        } else {
            const char* type_name = (t >= 0) ? ((lua_typename_t)LUA_TYPENAME)(L, t) : "?";
            printf("[Hook-NATIVE-PCALL-ERROR] code=%d errtype=%s\n", result, type_name ? type_name : "?");
        }
    }

    return result;
}

static int __cdecl detoured_luaL_loadbuffer(lua_State* L, const char* buff, int sz, const char* name) {
    std::string effective_name = GetEffectiveChunkName(buff, sz, name);
    const char* load_name = effective_name.empty() ? name : effective_name.c_str();
    int result = original_luaL_loadbuffer(L, buff, sz, load_name);

    if (native_error_hooks_active.load() && result != 0) {
        int top = ((lua_gettop_t)LUA_GETTOP)(L);
        int t = (top > 0) ? ((lua_type_t)LUA_TYPE)(L, -1) : -1;
        const char* chunk_name = (load_name && load_name[0]) ? load_name : "?";
        if (t == 4) {
            const char* msg = ((lua_tolstring_t)LUA_TOLSTRING)(L, -1, nullptr);
            printf("[Hook-NATIVE-LOAD-ERROR] code=%d chunk=%s msg=%s\n",
                   result,
                   TruncateForLog(chunk_name, 120).c_str(),
                   TruncateForLog(msg).c_str());
        } else {
            const char* type_name = (t >= 0) ? ((lua_typename_t)LUA_TYPENAME)(L, t) : "?";
            printf("[Hook-NATIVE-LOAD-ERROR] code=%d chunk=%s errtype=%s\n",
                   result,
                   TruncateForLog(chunk_name, 120).c_str(),
                   type_name ? type_name : "?");
        }
    }

    return result;
}

static void LogHookStatus(const char* step, MH_STATUS status) {
    std::string line = std::string("[A_State] ") + step + " -> " + MH_StatusToString(status);
    Log(line);
}

void HooksInit() {
    Log("[A_State] Initializing MinHook...");
    LogHookStatus("MH_Initialize", MH_Initialize());
    LogHookStatus("MH_CreateHook lua_pcall", MH_CreateHook((LPVOID)LUA_PCALL, &detoured_lua_pcall, (LPVOID*)&original_lua_pcall));
    LogHookStatus("MH_CreateHook luaL_loadbuffer", MH_CreateHook((LPVOID)LUAL_LOADBUFFER, &detoured_luaL_loadbuffer, (LPVOID*)&original_luaL_loadbuffer));
    LogHookStatus("MH_EnableHook lua_pcall", MH_EnableHook((LPVOID)LUA_PCALL));
    LogHookStatus("MH_EnableHook luaL_loadbuffer", MH_EnableHook((LPVOID)LUAL_LOADBUFFER));
}

void HooksShutdown() {
    LogHookStatus("MH_DisableHook luaL_loadbuffer", MH_DisableHook((LPVOID)LUAL_LOADBUFFER));
    LogHookStatus("MH_DisableHook lua_pcall", MH_DisableHook((LPVOID)LUA_PCALL));
    LogHookStatus("MH_Uninitialize", MH_Uninitialize());
}
