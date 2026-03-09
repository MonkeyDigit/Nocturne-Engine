#include <unordered_set>
#include <optional>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include "EventManager.h"
#include "StateManager.h"
#include "EngineLog.h"

// --- TRANSLATION DICTIONARIES ---
// Using an anonymous namespace to confine this data in this file only
// It gets allocated in memory only once on startup
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

    std::optional<EventType> ResolvePolledEventType(const sf::Event& event)
    {
        if (event.is<sf::Event::KeyPressed>()) return EventType::KeyDown;
        if (event.is<sf::Event::KeyReleased>()) return EventType::KeyUp;
        if (event.is<sf::Event::MouseButtonPressed>()) return EventType::MouseClick;
        if (event.is<sf::Event::MouseButtonReleased>()) return EventType::MouseRelease;
        if (event.is<sf::Event::MouseWheelScrolled>()) return EventType::MouseWheel;
        if (event.is<sf::Event::Resized>()) return EventType::Resized;
        if (event.is<sf::Event::TextEntered>()) return EventType::TextEntered;
        if (event.is<sf::Event::Closed>()) return EventType::Closed;
        return std::nullopt;
    }

    inline void MarkBindingMatched(Binding& bind, int keyCode)
    {
        // Store the first matched key/button code for the callback payload
        if (bind.m_details.m_keyCode == -1)
            bind.m_details.m_keyCode = keyCode;

        ++bind.m_evCount;
    }
}

// --- EVENT DETAILS ---
EventDetails::EventDetails(const std::string& bindName) : m_name(bindName) { Clear(); }

void EventDetails::Clear()
{
    m_size = sf::Vector2i(0, 0);
    m_textEntered = 0;
    m_mouse = sf::Vector2i(0, 0);
    m_mouseWheelDelta = 0.0f;
    m_keyCode = -1;
}

// --- BINDING ---
Binding::Binding(const std::string& bindName) : m_name(bindName), m_details(bindName), m_evCount(0) {}

void Binding::BindEvent(EventType type, int code)
{
    m_events.emplace_back(type, code);
}

// --- EVENT MANAGER ---
EventManager::EventManager()
    : m_currentState(StateType::Global),
    m_hasFocus(true)
{
    LoadBindings("config/bindings.cfg");
}

bool EventManager::AddBinding(std::unique_ptr<Binding> binding)
{
    if (!binding || m_bindings.find(binding->m_name) != m_bindings.end()) 
        return false;

    return m_bindings.emplace(binding->m_name, std::move(binding)).second;
}

bool EventManager::RemoveBinding(const std::string& name)
{
    return m_bindings.erase(name) > 0;
}

void EventManager::SetFocus(bool focus) { m_hasFocus = focus; }
sf::Vector2i EventManager::GetMousePos() const { return sf::Mouse::getPosition(); }
sf::Vector2i EventManager::GetMousePos(const sf::RenderWindow& window) const { return sf::Mouse::getPosition(window); }

bool EventManager::RemoveCallback(StateType state, const std::string& name)
{
    auto itr = m_callbacks.find(state);
    if (itr == m_callbacks.end()) return false;
    return itr->second.erase(name) > 0;
}

void EventManager::SetCurrentState(StateType type) { m_currentState = type; }

// --- PARSERS FOR HUMAN READABLE CONFIG FILES ---
int EventManager::ParseEventInfo(EventType evtype, const std::string& evinfoStr)
{
    const std::string normalized = ToLowerCopy(evinfoStr);

    switch (evtype)
    {
    case EventType::KeyDown: case EventType::KeyUp:
    case EventType::KeyboardHeld: case EventType::Keyboard:
    {
        auto it = STRING_TO_KEY_MAP.find(normalized);
        if (it != STRING_TO_KEY_MAP.end()) return static_cast<int>(it->second);
        break;
    }
    case EventType::MouseClick: case EventType::MouseRelease:
    case EventType::MouseHeld: case EventType::Mouse:
    {
        auto it = STRING_TO_MOUSE_MAP.find(normalized);
        if (it != STRING_TO_MOUSE_MAP.end()) return static_cast<int>(it->second);
        break;
    }
    default:
        break;
    }

    // Fallback to numeric code (strict: full string must be numeric)
    try
    {
        size_t parsedChars = 0;
        const int value = std::stoi(evinfoStr, &parsedChars);
        if (parsedChars != evinfoStr.size())
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

void EventManager::ProcessPolledEvent(const sf::Event& event)
{
    const std::optional<EventType> currentType = ResolvePolledEventType(event);
    if (!currentType) return;

    for (auto& [_, bindPtr] : m_bindings)
    {
        Binding& bind = *bindPtr;

        for (const auto& [eventType, code] : bind.m_events)
        {
            if (eventType != *currentType) continue;

            switch (eventType)
            {
            case EventType::KeyDown:
            {
                const auto* keyEvent = event.getIf<sf::Event::KeyPressed>();
                if (keyEvent && code == static_cast<int>(keyEvent->code))
                    MarkBindingMatched(bind, code);
                break;
            }
            case EventType::KeyUp:
            {
                const auto* keyEvent = event.getIf<sf::Event::KeyReleased>();
                if (keyEvent && code == static_cast<int>(keyEvent->code))
                    MarkBindingMatched(bind, code);
                break;
            }
            case EventType::MouseClick:
            {
                const auto* mouseEvent = event.getIf<sf::Event::MouseButtonPressed>();
                if (mouseEvent && code == static_cast<int>(mouseEvent->button))
                {
                    bind.m_details.m_mouse.x = mouseEvent->position.x;
                    bind.m_details.m_mouse.y = mouseEvent->position.y;
                    MarkBindingMatched(bind, code);
                }
                break;
            }
            case EventType::MouseRelease:
            {
                const auto* mouseEvent = event.getIf<sf::Event::MouseButtonReleased>();
                if (mouseEvent && code == static_cast<int>(mouseEvent->button))
                {
                    bind.m_details.m_mouse.x = mouseEvent->position.x;
                    bind.m_details.m_mouse.y = mouseEvent->position.y;
                    MarkBindingMatched(bind, code);
                }
                break;
            }
            case EventType::MouseWheel:
            {
                const auto* mouseEvent = event.getIf<sf::Event::MouseWheelScrolled>();
                if (mouseEvent && code == static_cast<int>(mouseEvent->wheel))
                {
                    bind.m_details.m_mouse.x = mouseEvent->position.x;
                    bind.m_details.m_mouse.y = mouseEvent->position.y;
                    bind.m_details.m_mouseWheelDelta = mouseEvent->delta;
                    MarkBindingMatched(bind, code);
                }
                break;
            }
            case EventType::Resized:
            {
                const auto* windowEvent = event.getIf<sf::Event::Resized>();
                if (windowEvent)
                {
                    bind.m_details.m_size.x = windowEvent->size.x;
                    bind.m_details.m_size.y = windowEvent->size.y;
                    MarkBindingMatched(bind, code);
                }
                break;
            }
            case EventType::TextEntered:
            {
                const auto* textEvent = event.getIf<sf::Event::TextEntered>();
                if (textEvent)
                {
                    bind.m_details.m_textEntered = textEvent->unicode;
                    MarkBindingMatched(bind, code);
                }
                break;
            }
            case EventType::Closed:
                ++bind.m_evCount;
                break;
            default:
                ++bind.m_evCount;
                break;
            }
        }
    }
}

void EventManager::ProcessRealTimeInput()
{
    if (!m_hasFocus) return;

    for (auto& [_, bindPtr] : m_bindings)
    {
        Binding& bind = *bindPtr;

        for (const auto& [eventType, code] : bind.m_events)
        {
            switch (eventType)
            {
            case EventType::Keyboard:
            case EventType::KeyboardHeld:
                if (sf::Keyboard::isKeyPressed(static_cast<sf::Keyboard::Key>(code)))
                    MarkBindingMatched(bind, code);
                break;

            case EventType::Mouse:
            case EventType::MouseHeld:
                if (sf::Mouse::isButtonPressed(static_cast<sf::Mouse::Button>(code)))
                    MarkBindingMatched(bind, code);
                break;

            default:
                break;
            }
        }
    }
}

// At this point, if the matched events counter is equal to the number of the binding's triggering events, its callback function is called
void EventManager::DispatchCallbacks()
{
    for (auto& b_itr : m_bindings)  // Iterate through the bindings
    {
        Binding* bind = b_itr.second.get();

        if (bind->m_events.size() == bind->m_evCount)
        {   // Using a lambda function
            auto dispatchForState = [&](StateType state)
                {
                    auto callbacksIt = m_callbacks.find(state);
                    if (callbacksIt == m_callbacks.end()) return;

                    auto callItr = callbacksIt->second.find(bind->m_name);
                    if (callItr != callbacksIt->second.end())
                    {   // Pass in information about events
                        callItr->second(bind->m_details);
                    }
                };

            dispatchForState(m_currentState);

            // Also dispatch global callbacks, but avoid duplicate dispatch
            if (m_currentState != StateType::Global)
                dispatchForState(StateType::Global);
        }

        bind->m_evCount = 0;
        bind->m_details.Clear();
    }
}