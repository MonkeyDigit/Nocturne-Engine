#include <algorithm>
#include <cctype>
#include <fstream>
#include <limits>
#include <sstream>
#include <unordered_set>

#include "Window.h"
#include "EngineLog.h"
#include "Utilities.h"
#include "ConfigParseUtils.h"

namespace
{
    using ParseUtils::TryReadExact;
    using ParseUtils::PrepareConfigLine;

    std::string ToLowerCopy(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return value;
    }

    bool TryParseLogLevel(const std::string& raw, EngineLog::Level& outLevel)
    {
        const std::string value = ToLowerCopy(raw);
        if (value == "info") { outLevel = EngineLog::Level::Info; return true; }
        if (value == "warn" || value == "warning") { outLevel = EngineLog::Level::Warn; return true; }
        if (value == "error") { outLevel = EngineLog::Level::Error; return true; }
        return false;
    }

    std::string CanonicalWindowKey(std::string key)
    {
        key = ToLowerCopy(std::move(key));
        if (key == "resizeable") return "resizable";
        if (key == "windowsize") return "windowres";
        return key;
    }

    void WarnWindowConfigLine(unsigned int lineNumber, const std::string& message)
    {
        EngineLog::Warn("window.cfg line " + std::to_string(lineNumber) + ": " + message);
    }
}

void Window::LoadConfig()
{
    // Engine-safe defaults
    m_gameResolution = { 640.0f, 360.0f };
    m_uiResolution = { 1280.0f, 720.0f };

    std::ifstream file(Utils::GetWorkingDirectory() + "config/window.cfg");
    if (!file.is_open())
    {
        EngineLog::WarnOnce("window.config.missing", "Failed to load window.cfg. Using defaults.");
        return;
    }

    std::unordered_set<std::string> seenKeys;
    std::string line;
    unsigned int lineNumber = 0;

    while (std::getline(file, line))
    {
        ++lineNumber;

        if (!PrepareConfigLine(line)) continue;

        std::stringstream keystream(line);
        std::string rawType;
        if (!(keystream >> rawType)) continue;

        const std::string type = CanonicalWindowKey(rawType);

        if (!seenKeys.insert(type).second)
        {
            WarnWindowConfigLine(
                lineNumber,
                "Duplicate key '" + rawType + "'. Last valid value wins.");
        }

        if (type == "gameres")
        {
            float x = 0.0f, y = 0.0f;
            if (!TryReadExact(keystream, x, y) || x <= 0.0f || y <= 0.0f)
            {
                WarnWindowConfigLine(lineNumber, "Invalid GameRes (expected: GameRes <x> <y>, both > 0).");
                continue;
            }
            m_gameResolution = { x, y };
        }
        else if (type == "uires")
        {
            float x = 0.0f, y = 0.0f;
            if (!TryReadExact(keystream, x, y) || x <= 0.0f || y <= 0.0f)
            {
                WarnWindowConfigLine(lineNumber, "Invalid UIRes (expected: UIRes <x> <y>, both > 0).");
                continue;
            }
            m_uiResolution = { x, y };
        }
        else if (type == "loglevel")
        {
            std::string levelStr;
            if (!TryReadExact(keystream, levelStr))
            {
                WarnWindowConfigLine(lineNumber, "Invalid LogLevel (expected: LogLevel Info|Warn|Error).");
                continue;
            }

            EngineLog::Level parsedLevel;
            if (!TryParseLogLevel(levelStr, parsedLevel))
            {
                WarnWindowConfigLine(lineNumber, "Unknown LogLevel '" + levelStr + "' (use Info/Warn/Error).");
                continue;
            }

            EngineLog::SetMinLevel(parsedLevel);
        }
        else if (type == "frameratelimit")
        {
            int limit = 0;
            if (!TryReadExact(keystream, limit) || limit < 0)
            {
                WarnWindowConfigLine(lineNumber, "Invalid FrameRateLimit (expected int >= 0, 0 = unlimited).");
                continue;
            }

            m_frameRateLimit = limit;
        }
        else if (type == "fullscreen")
        {
            int value = 0;
            if (!TryReadExact(keystream, value) || (value != 0 && value != 1))
            {
                WarnWindowConfigLine(lineNumber, "Invalid Fullscreen (expected 0 or 1).");
                continue;
            }

            m_isFullscreen = (value == 1);
        }
        else if (type == "resizable")
        {
            int value = 0;
            if (!TryReadExact(keystream, value) || (value != 0 && value != 1))
            {
                WarnWindowConfigLine(lineNumber, "Invalid Resizable (expected 0 or 1).");
                continue;
            }

            m_isResizeable = (value == 1);
        }
        else if (type == "windowres")
        {
            unsigned int x = 0, y = 0;
            if (!TryReadExact(keystream, x, y) || x == 0 || y == 0)
            {
                WarnWindowConfigLine(lineNumber, "Invalid WindowRes (expected: WindowRes <x> <y>, both > 0).");
                continue;
            }

            m_windowSize = { x, y };
        }
        else if (type == "aiseed")
        {
            std::string seedToken;
            if (!TryReadExact(keystream, seedToken))
            {
                WarnWindowConfigLine(lineNumber, "Invalid AISeed (expected: AISeed Auto|Random|<uint32>).");
                continue;
            }

            const std::string normalizedSeed = ToLowerCopy(seedToken);
            if (normalizedSeed == "auto" || normalizedSeed == "random")
            {
                m_hasFixedAISeed = false;
                m_fixedAISeed = 0;
            }
            else
            {
                try
                {
                    size_t parsedChars = 0;
                    const unsigned long long parsed = std::stoull(seedToken, &parsedChars);

                    if (parsedChars != seedToken.size() ||
                        parsed > std::numeric_limits<std::uint32_t>::max())
                    {
                        WarnWindowConfigLine(lineNumber, "AISeed out of range (expected uint32).");
                        continue;
                    }

                    m_fixedAISeed = static_cast<std::uint32_t>(parsed);
                    m_hasFixedAISeed = true;
                }
                catch (...)
                {
                    WarnWindowConfigLine(lineNumber, "Invalid AISeed '" + seedToken + "' (use Auto/Random or uint32).");
                    continue;
                }
            }
        }
        else
        {
            WarnWindowConfigLine(lineNumber, "Unknown key '" + rawType + "'.");
        }
    }
}