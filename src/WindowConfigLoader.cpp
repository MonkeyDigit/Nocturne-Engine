#include <algorithm>
#include <cctype>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

#include "Window.h"
#include "EngineLog.h"
#include "Utilities.h"
#include "ConfigParseUtils.h"

namespace
{
    using ParseUtils::PrepareConfigLine;
    using ParseUtils::TryReadExact;

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

    bool TryParsePositiveFloatPair(std::stringstream& keystream, float& x, float& y)
    {
        return TryReadExact(keystream, x, y) && x > 0.0f && y > 0.0f;
    }

    bool TryParsePositiveUIntPair(std::stringstream& keystream, unsigned int& x, unsigned int& y)
    {
        return TryReadExact(keystream, x, y) && x > 0u && y > 0u;
    }

    bool TryParseNonNegativeInt(std::stringstream& keystream, int& value)
    {
        return TryReadExact(keystream, value) && value >= 0;
    }

    bool TryParseBinaryFlag(std::stringstream& keystream, bool& outValue)
    {
        int value = 0;
        if (!TryReadExact(keystream, value)) return false;
        if (value != 0 && value != 1) return false;
        outValue = (value == 1);
        return true;
    }

    enum class AISeedParseResult
    {
        Success,
        Invalid,
        OutOfRange
    };

    AISeedParseResult TryParseAISeedToken(
        const std::string& seedToken,
        bool& outHasFixedSeed,
        std::uint32_t& outFixedSeed)
    {
        const std::string normalizedSeed = ToLowerCopy(seedToken);
        if (normalizedSeed == "auto" || normalizedSeed == "random")
        {
            outHasFixedSeed = false;
            outFixedSeed = 0;
            return AISeedParseResult::Success;
        }

        try
        {
            size_t parsedChars = 0;
            const unsigned long long parsed = std::stoull(seedToken, &parsedChars);

            if (parsedChars != seedToken.size())
                return AISeedParseResult::Invalid;

            if (parsed > std::numeric_limits<std::uint32_t>::max())
                return AISeedParseResult::OutOfRange;

            outFixedSeed = static_cast<std::uint32_t>(parsed);
            outHasFixedSeed = true;
            return AISeedParseResult::Success;
        }
        catch (const std::out_of_range&)
        {
            return AISeedParseResult::OutOfRange;
        }
        catch (...)
        {
            return AISeedParseResult::Invalid;
        }
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
            float x = 0.0f;
            float y = 0.0f;
            if (!TryParsePositiveFloatPair(keystream, x, y))
            {
                WarnWindowConfigLine(lineNumber, "Invalid GameRes (expected: GameRes <x> <y>, both > 0).");
                continue;
            }

            m_gameResolution = { x, y };
        }
        else if (type == "uires")
        {
            float x = 0.0f;
            float y = 0.0f;
            if (!TryParsePositiveFloatPair(keystream, x, y))
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
            if (!TryParseNonNegativeInt(keystream, limit))
            {
                WarnWindowConfigLine(lineNumber, "Invalid FrameRateLimit (expected int >= 0, 0 = unlimited).");
                continue;
            }

            m_frameRateLimit = limit;
        }
        else if (type == "fullscreen")
        {
            bool value = false;
            if (!TryParseBinaryFlag(keystream, value))
            {
                WarnWindowConfigLine(lineNumber, "Invalid Fullscreen (expected 0 or 1).");
                continue;
            }

            m_isFullscreen = value;
        }
        else if (type == "resizable")
        {
            bool value = false;
            if (!TryParseBinaryFlag(keystream, value))
            {
                WarnWindowConfigLine(lineNumber, "Invalid Resizable (expected 0 or 1).");
                continue;
            }

            m_isResizeable = value;
        }
        else if (type == "windowres")
        {
            unsigned int x = 0u;
            unsigned int y = 0u;
            if (!TryParsePositiveUIntPair(keystream, x, y))
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

            const AISeedParseResult result =
                TryParseAISeedToken(seedToken, m_hasFixedAISeed, m_fixedAISeed);

            if (result == AISeedParseResult::OutOfRange)
            {
                WarnWindowConfigLine(lineNumber, "AISeed out of range (expected uint32).");
                continue;
            }

            if (result == AISeedParseResult::Invalid)
            {
                WarnWindowConfigLine(lineNumber, "Invalid AISeed '" + seedToken + "' (use Auto/Random or uint32).");
                continue;
            }
        }
        else
        {
            WarnWindowConfigLine(lineNumber, "Unknown key '" + rawType + "'.");
        }
    }
}