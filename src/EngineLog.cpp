#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_set>
#include "EngineLog.h"

namespace
{
    std::unordered_set<std::string> g_onceKeys;
    std::mutex g_onceMutex;
    std::atomic<EngineLog::Level> g_minLevel{ EngineLog::Level::Info };

    bool ShouldLog(EngineLog::Level level)
    {
        const auto minLevel = g_minLevel.load(std::memory_order_relaxed);
        return static_cast<int>(level) >= static_cast<int>(minLevel);
    }

    std::string MakeTimestamp()
    {
        using clock = std::chrono::system_clock;
        const auto now = clock::now();
        const std::time_t tt = clock::to_time_t(now);

        std::tm localTm{};
#ifdef _WIN32
        localtime_s(&localTm, &tt);
#else
        localtime_r(&tt, &localTm);
#endif

        std::ostringstream os;
        os << std::put_time(&localTm, "%H:%M:%S");
        return os.str();
    }

    void PrintLine(const char* prefix, std::string_view message)
    {
        std::cerr << "[" << MakeTimestamp() << "] " << prefix << ' ' << message << '\n';
    }

    bool LogOnceImpl(EngineLog::Level level, const char* prefix, std::string_view key, std::string_view message)
    {
        if (!ShouldLog(level)) return false;

        std::lock_guard<std::mutex> lock(g_onceMutex);
        auto [_, inserted] = g_onceKeys.emplace(key);
        if (!inserted) return false;

        PrintLine(prefix, message);
        return true;
    }
}

void EngineLog::Warn(std::string_view message)
{
    if (!ShouldLog(Level::Warn)) return;
    PrintLine("[WARN]", message);
}

void EngineLog::Error(std::string_view message)
{
    if (!ShouldLog(Level::Error)) return;
    PrintLine("[ERROR]", message);
}

bool EngineLog::WarnOnce(std::string_view key, std::string_view message)
{
    return LogOnceImpl(Level::Warn, "[WARN]", key, message);
}

bool EngineLog::ErrorOnce(std::string_view key, std::string_view message)
{
    return LogOnceImpl(Level::Error, "[ERROR]", key, message);
}

void EngineLog::Info(std::string_view message)
{
    if (!ShouldLog(Level::Info)) return;
    PrintLine("[INFO]", message);
}

bool EngineLog::InfoOnce(std::string_view key, std::string_view message)
{
    return LogOnceImpl(Level::Info, "[INFO]", key, message);
}

void EngineLog::ResetOnce(std::string_view key)
{
    std::lock_guard<std::mutex> lock(g_onceMutex);
    g_onceKeys.erase(std::string(key));
}

void EngineLog::ResetAllOnce()
{
    std::lock_guard<std::mutex> lock(g_onceMutex);
    g_onceKeys.clear();
}

void EngineLog::SetMinLevel(Level level)
{
    g_minLevel.store(level, std::memory_order_relaxed);
}

EngineLog::Level EngineLog::GetMinLevel()
{
    return g_minLevel.load(std::memory_order_relaxed);
}