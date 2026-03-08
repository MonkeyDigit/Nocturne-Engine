#pragma once
#include <string_view>

namespace EngineLog
{
    void Warn(std::string_view message);
    void Error(std::string_view message);

    // Returns true if the message was printed, false if it was suppressed
    bool WarnOnce(std::string_view key, std::string_view message);
    bool ErrorOnce(std::string_view key, std::string_view message);
    void Info(std::string_view message);
    bool InfoOnce(std::string_view key, std::string_view message);

    void ResetOnce(std::string_view key);
    void ResetAllOnce();

    enum class Level
    {
        Info = 0,
        Warn = 1,
        Error = 2
    };

    void SetMinLevel(Level level);
    Level GetMinLevel();
}