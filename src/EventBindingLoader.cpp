#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "EventManager.h"
#include "EngineLog.h"
#include "ConfigParseUtils.h"

namespace
{
    std::string ToLowerCopy(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return value;
    }

    void WarnBindingLine(unsigned int lineNumber, const std::string& message)
    {
        EngineLog::Warn("bindings.cfg line " + std::to_string(lineNumber) + ": " + message);
    }

    bool TrySplitEventToken(const std::string& token, std::string& outType, std::string& outInfo)
    {
        const size_t colonPos = token.find(':');
        if (colonPos == std::string::npos) return false;
        if (colonPos == 0 || colonPos == token.size() - 1) return false;
        if (token.find(':', colonPos + 1) != std::string::npos) return false; // Only one ':'

        outType = token.substr(0, colonPos);
        outInfo = token.substr(colonPos + 1);
        return true;
    }

    bool TryParseNonNegativeIntExact(const std::string& raw, int& outValue)
    {
        try
        {
            size_t parsedChars = 0;
            const long long parsed = std::stoll(raw, &parsedChars);
            if (parsedChars != raw.size()) return false;
            if (parsed < 0) return false;
            if (parsed > std::numeric_limits<int>::max()) return false;

            outValue = static_cast<int>(parsed);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    const std::unordered_map<std::string, EventType> kStringToEventMap = {
        {"closed", EventType::Closed}, {"resized", EventType::Resized},
        {"focuslost", EventType::FocusLost}, {"focusgained", EventType::FocusGained},
        {"textentered", EventType::TextEntered}, {"keydown", EventType::KeyDown},
        {"keyup", EventType::KeyUp}, {"mousewheel", EventType::MouseWheel},
        {"mouseclick", EventType::MouseClick}, {"mouserelease", EventType::MouseRelease},
        {"keyboardheld", EventType::KeyboardHeld}, {"mouseheld", EventType::MouseHeld},
        {"keyboard", EventType::Keyboard}, {"mouse", EventType::Mouse},
        {"joystick", EventType::Joystick}
    };

    const std::unordered_map<std::string, sf::Keyboard::Key> kStringToKeyMap = {
        {"a", sf::Keyboard::Key::A}, {"b", sf::Keyboard::Key::B}, {"c", sf::Keyboard::Key::C},
        {"d", sf::Keyboard::Key::D}, {"e", sf::Keyboard::Key::E}, {"f", sf::Keyboard::Key::F},
        {"g", sf::Keyboard::Key::G}, {"h", sf::Keyboard::Key::H}, {"i", sf::Keyboard::Key::I},
        {"j", sf::Keyboard::Key::J}, {"k", sf::Keyboard::Key::K}, {"l", sf::Keyboard::Key::L},
        {"m", sf::Keyboard::Key::M}, {"n", sf::Keyboard::Key::N}, {"o", sf::Keyboard::Key::O},
        {"p", sf::Keyboard::Key::P}, {"q", sf::Keyboard::Key::Q}, {"r", sf::Keyboard::Key::R},
        {"s", sf::Keyboard::Key::S}, {"t", sf::Keyboard::Key::T}, {"u", sf::Keyboard::Key::U},
        {"v", sf::Keyboard::Key::V}, {"w", sf::Keyboard::Key::W}, {"x", sf::Keyboard::Key::X},
        {"y", sf::Keyboard::Key::Y}, {"z", sf::Keyboard::Key::Z},

        {"f1", sf::Keyboard::Key::F1}, {"f2", sf::Keyboard::Key::F2}, {"f3", sf::Keyboard::Key::F3},
        {"f4", sf::Keyboard::Key::F4}, {"f5", sf::Keyboard::Key::F5}, {"f6", sf::Keyboard::Key::F6},
        {"f7", sf::Keyboard::Key::F7}, {"f8", sf::Keyboard::Key::F8}, {"f9", sf::Keyboard::Key::F9},
        {"f10", sf::Keyboard::Key::F10}, {"f11", sf::Keyboard::Key::F11}, {"f12", sf::Keyboard::Key::F12},

        {"escape", sf::Keyboard::Key::Escape}, {"esc", sf::Keyboard::Key::Escape},
        {"space", sf::Keyboard::Key::Space}, {"spacebar", sf::Keyboard::Key::Space},
        {"return", sf::Keyboard::Key::Enter}, {"enter", sf::Keyboard::Key::Enter},
        {"lshift", sf::Keyboard::Key::LShift}, {"rshift", sf::Keyboard::Key::RShift},
        {"lcontrol", sf::Keyboard::Key::LControl}, {"ctrl", sf::Keyboard::Key::LControl},
        {"tab", sf::Keyboard::Key::Tab},

        {"left", sf::Keyboard::Key::Left}, {"right", sf::Keyboard::Key::Right},
        {"up", sf::Keyboard::Key::Up}, {"down", sf::Keyboard::Key::Down}
    };

    const std::unordered_map<std::string, sf::Mouse::Button> kStringToMouseMap = {
        {"left", sf::Mouse::Button::Left}, {"right", sf::Mouse::Button::Right}, {"middle", sf::Mouse::Button::Middle},
        {"lmb", sf::Mouse::Button::Left}, {"rmb", sf::Mouse::Button::Right}, {"mmb", sf::Mouse::Button::Middle}
    };

    bool TryParseEventType(const std::string& rawType, EventType& outType)
    {
        const auto it = kStringToEventMap.find(ToLowerCopy(rawType));
        if (it == kStringToEventMap.end()) return false;
        outType = it->second;
        return true;
    }

    bool TryParseEventCode(EventType evtype, const std::string& evinfoStr, int& outCode)
    {
        const std::string normalized = ToLowerCopy(evinfoStr);

        switch (evtype)
        {
        case EventType::KeyDown:
        case EventType::KeyUp:
        case EventType::KeyboardHeld:
        case EventType::Keyboard:
        {
            const auto it = kStringToKeyMap.find(normalized);
            if (it == kStringToKeyMap.end()) return false;
            outCode = static_cast<int>(it->second);
            return true;
        }

        case EventType::MouseClick:
        case EventType::MouseRelease:
        case EventType::MouseHeld:
        case EventType::Mouse:
        {
            const auto it = kStringToMouseMap.find(normalized);
            if (it == kStringToMouseMap.end()) return false;
            outCode = static_cast<int>(it->second);
            return true;
        }

        case EventType::MouseWheel:
            if (normalized == "vertical" || normalized == "v" || normalized == "0") { outCode = 0; return true; }
            if (normalized == "horizontal" || normalized == "h" || normalized == "1") { outCode = 1; return true; }
            return false;

        case EventType::Closed:
        case EventType::Resized:
        case EventType::FocusLost:
        case EventType::FocusGained:
        case EventType::TextEntered:
            if (normalized == "0" || normalized == "none" || normalized == "any" || normalized == "_")
            {
                outCode = 0;
                return true;
            }
            return false;

        case EventType::Joystick:
            return TryParseNonNegativeIntExact(evinfoStr, outCode);
        }

        return false;
    }

    std::uint64_t MakeEventDedupKey(EventType eventType, int code)
    {
        const std::uint64_t typeBits = static_cast<std::uint64_t>(static_cast<std::uint32_t>(eventType));
        const std::uint64_t codeBits = static_cast<std::uint64_t>(static_cast<std::uint32_t>(code));
        return (typeBits << 32) | codeBits;
    }
}

// --- PARSERS FOR HUMAN READABLE CONFIG FILES ---
int EventManager::ParseEventInfo(EventType evtype, const std::string& evinfoStr)
{
    int parsedCode = 0;
    return TryParseEventCode(evtype, evinfoStr, parsedCode) ? parsedCode : -1;
}

void EventManager::LoadBindings(const std::string& path)
{
    std::ifstream bindings{ Utils::GetWorkingDirectory() + path };

    if (!bindings.is_open())
    {
        EngineLog::Error("Failed loading bindings file: " + path);
        return;
    }

    std::string line;
    unsigned int lineNumber = 0;

    while (std::getline(bindings, line))
    {
        ++lineNumber;

        if (!ParseUtils::PrepareConfigLine(line)) continue;

        std::stringstream keystream(line);

        std::string callbackName;
        if (!(keystream >> callbackName) || callbackName.empty())
        {
            WarnBindingLine(lineNumber, "Missing callback name.");
            continue;
        }

        auto bind = std::make_unique<Binding>(callbackName);
        std::unordered_set<std::uint64_t> uniqueEvents;
        std::string eventToken;

        while (keystream >> eventToken)
        {
            std::string evtypeStr;
            std::string evinfoStr;
            if (!TrySplitEventToken(eventToken, evtypeStr, evinfoStr))
            {
                WarnBindingLine(lineNumber, "Invalid token '" + eventToken + "' (expected EventType:EventInfo).");
                continue;
            }

            EventType evtype;
            if (!TryParseEventType(evtypeStr, evtype))
            {
                WarnBindingLine(lineNumber, "Unknown event type '" + evtypeStr + "'.");
                continue;
            }

            const int code = ParseEventInfo(evtype, evinfoStr);
            if (code < 0)
            {
                WarnBindingLine(lineNumber, "Invalid event info '" + evinfoStr + "' for token '" + eventToken + "'.");
                continue;
            }

            const std::uint64_t dedupKey = MakeEventDedupKey(evtype, code);
            if (!uniqueEvents.insert(dedupKey).second)
            {
                WarnBindingLine(lineNumber, "Duplicate event '" + eventToken + "' in binding '" + callbackName + "'.");
                continue;
            }

            bind->BindEvent(evtype, code);
        }

        if (bind->m_events.empty())
        {
            WarnBindingLine(lineNumber, "Binding '" + callbackName + "' has no valid events and will be skipped.");
            continue;
        }

        if (!AddBinding(std::move(bind)))
        {
            WarnBindingLine(lineNumber, "Duplicate binding name '" + callbackName + "' ignored.");
        }
    }
}