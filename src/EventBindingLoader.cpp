#include <unordered_set>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include "EventManager.h"
#include "EngineLog.h"

// after includes
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

    const std::unordered_map<std::string, EventType> STRING_TO_EVENT_MAP = {
        {"closed", EventType::Closed}, {"resized", EventType::Resized},
        {"focuslost", EventType::FocusLost}, {"focusgained", EventType::FocusGained},
        {"textentered", EventType::TextEntered}, {"keydown", EventType::KeyDown},
        {"keyup", EventType::KeyUp}, {"mousewheel", EventType::MouseWheel},
        {"mouseclick", EventType::MouseClick}, {"mouserelease", EventType::MouseRelease},
        {"keyboardheld", EventType::KeyboardHeld}, {"mouseheld", EventType::MouseHeld},
        {"keyboard", EventType::Keyboard}, {"mouse", EventType::Mouse},
        {"joystick", EventType::Joystick}
    };

    const std::unordered_map<std::string, sf::Keyboard::Key> STRING_TO_KEY_MAP = {
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

    const std::unordered_map<std::string, sf::Mouse::Button> STRING_TO_MOUSE_MAP = {
        {"left", sf::Mouse::Button::Left}, {"right", sf::Mouse::Button::Right}, {"middle", sf::Mouse::Button::Middle},
        {"lmb", sf::Mouse::Button::Left}, {"rmb", sf::Mouse::Button::Right}, {"mmb", sf::Mouse::Button::Middle}
    };

}

// --- PARSERS FOR HUMAN READABLE CONFIG FILES ---
int EventManager::ParseEventInfo(EventType evtype, const std::string& evinfoStr)
{
    const std::string normalized = ToLowerCopy(evinfoStr);

    switch (evtype)
    {
    case EventType::KeyDown:
    case EventType::KeyUp:
    case EventType::KeyboardHeld:
    case EventType::Keyboard:
    {
        auto it = STRING_TO_KEY_MAP.find(normalized);
        return (it != STRING_TO_KEY_MAP.end()) ? static_cast<int>(it->second) : -1;
    }

    case EventType::MouseClick:
    case EventType::MouseRelease:
    case EventType::MouseHeld:
    case EventType::Mouse:
    {
        auto it = STRING_TO_MOUSE_MAP.find(normalized);
        return (it != STRING_TO_MOUSE_MAP.end()) ? static_cast<int>(it->second) : -1;
    }

    case EventType::MouseWheel:
        // Keep config human-readable and stable.
        if (normalized == "vertical" || normalized == "v" || normalized == "0") return 0;
        if (normalized == "horizontal" || normalized == "h" || normalized == "1") return 1;
        return -1;

    case EventType::Closed:
    case EventType::Resized:
    case EventType::FocusLost:
    case EventType::FocusGained:
    case EventType::TextEntered:
        // These events do not use a code payload
        if (normalized == "0" || normalized == "none" || normalized == "any" || normalized == "_")
            return 0;
        return -1;

    case EventType::Joystick:
        break;
    }

    // Numeric fallback only for joystick-like future extensions
    try
    {
        size_t parsedChars = 0;
        const int value = std::stoi(evinfoStr, &parsedChars);
        if (parsedChars != evinfoStr.size() || value < 0)
            return -1;
        return value;
    }
    catch (...)
    {
        return -1;
    }
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

        // Support inline comments and full-line comments.
        const size_t commentPos = line.find('#');
        if (commentPos != std::string::npos)
            line.erase(commentPos);

        const size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) continue; // empty/whitespace
        if (line[first] == '|') continue;         // custom comment marker

        std::stringstream keystream(line);

        std::string callbackName;
        if (!(keystream >> callbackName) || callbackName.empty())
        {
            WarnBindingLine(lineNumber, "Missing callback name.");
            continue;
        }

        auto bind = std::make_unique<Binding>(callbackName);
        std::unordered_set<std::string> uniqueEvents;
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

            const std::string evtypeNorm = ToLowerCopy(evtypeStr);
            auto typeIt = STRING_TO_EVENT_MAP.find(evtypeNorm);
            if (typeIt == STRING_TO_EVENT_MAP.end())
            {
                WarnBindingLine(lineNumber, "Unknown event type '" + evtypeStr + "'.");
                continue;
            }

            const EventType evtype = typeIt->second;
            const int code = ParseEventInfo(evtype, evinfoStr);
            if (code < 0)
            {
                WarnBindingLine(lineNumber, "Invalid event info '" + evinfoStr + "' for token '" + eventToken + "'.");
                continue;
            }

            const std::string dedupKey =
                std::to_string(static_cast<int>(evtype)) + ":" + std::to_string(code);

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