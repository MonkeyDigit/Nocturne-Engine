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

    // KEYBOARD EVENT
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
    default: break;
    }

    // Fallback to int value
    try
    {
        return std::stoi(evinfoStr);
    }
    catch (...)
    {
        EngineLog::WarnOnce("bindings.stoi_error", "Error (stoi): impossible conversion: " + evinfoStr);
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

    std::string line;                       // Each line is a binding with the callback name and the events
    while (std::getline(bindings, line))
    {
        // Support inline comments and full-line comments
        const size_t commentPos = line.find('#');
        if (commentPos != std::string::npos)
            line.erase(commentPos);
        // Ignore empty lines
        const size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) continue;       // empty/whitespace
        if (line[first] == '|') continue;               // custom comment marker

        // GET CALLBACK NAME
        std::stringstream keystream(line);
        std::string callbackName;
        if (!(keystream >> callbackName) || callbackName.empty())
        {
            EngineLog::WarnOnce("bindings.invalid_line", "Invalid binding line: " + line);
            continue;
        }

        auto bind = std::make_unique<Binding>(callbackName);

        // READ EVENTS
        std::string eventToken;
        while (keystream >> eventToken)    // // e.g. "KeyDown:Left" or "Closed:0"
        {
            const size_t colonPos = eventToken.find(':');
            if (colonPos == std::string::npos)
            {
                EngineLog::WarnOnce("bindings.invalid_token", "Invalid token format: " + eventToken);
                continue;
            }

            std::string evtypeStr = eventToken.substr(0, colonPos);
            std::string evinfoStr = eventToken.substr(colonPos + 1);

            // String translation
            const std::string evtypeNorm = ToLowerCopy(evtypeStr);
            auto typeIt = STRING_TO_EVENT_MAP.find(evtypeNorm);
            if (typeIt == STRING_TO_EVENT_MAP.end())
            {
                EngineLog::WarnOnce("bindings.unknown_event_type", "Unknown event type in config file: " + evtypeStr);
                continue;
            }

            // BIND EVENT
            const EventType evtype = typeIt->second;
            const int code = ParseEventInfo(evtype, evinfoStr);
            if (code < 0)
            {
                EngineLog::WarnOnce("bindings.invalid_event_code", "Invalid event code for token: " + eventToken);
                continue;
            }
            bind->BindEvent(evtype, code);
        }

        if (bind->m_events.empty())
        {
            EngineLog::WarnOnce("bindings.empty_after_parse", "Skipping binding with no valid events: " + callbackName);
            continue;
        }

        if (!AddBinding(std::move(bind)))
            EngineLog::WarnOnce("bindings.duplicate_or_failed_insert", "Couldn't load binding: " + callbackName);
    }

    // bindings.close(); ISN'T NECESSARY as std::ifstream gets closed automatically when going out of scope
}

void EventManager::ProcessPolledEvent(const sf::Event& event)
{
    std::optional<EventType> currentSFMLEventType;
    if (event.is<sf::Event::KeyPressed>()) currentSFMLEventType = EventType::KeyDown;
    else if (event.is<sf::Event::KeyReleased>()) currentSFMLEventType = EventType::KeyUp;
    else if (event.is<sf::Event::MouseButtonPressed>()) currentSFMLEventType = EventType::MouseClick;
    else if (event.is<sf::Event::MouseButtonReleased>()) currentSFMLEventType = EventType::MouseRelease;
    else if (event.is<sf::Event::MouseWheelScrolled>()) currentSFMLEventType = EventType::MouseWheel;
    else if (event.is<sf::Event::Resized>()) currentSFMLEventType = EventType::Resized;
    else if (event.is<sf::Event::TextEntered>()) currentSFMLEventType = EventType::TextEntered;
    else if (event.is<sf::Event::Closed>()) currentSFMLEventType = EventType::Closed;

    if (!currentSFMLEventType.has_value()) return;

    for (auto& b_itr : m_bindings)              // Iterate through the bindings
    {
        Binding* bind = b_itr.second.get();
        for (auto& e_itr : bind->m_events)      // Iterate through the binding events
        {
            if (e_itr.first != currentSFMLEventType.value()) continue;

            switch (e_itr.first)
            {
                case EventType::KeyDown:
                {
                    const auto* keyEvent = event.getIf<sf::Event::KeyPressed>();
                    if (e_itr.second == static_cast<int>(keyEvent->code))
                    {
                        if (bind->m_details.m_keyCode == -1) bind->m_details.m_keyCode = e_itr.second;
                        ++(bind->m_evCount);
                    }
                    break;
                }
                case EventType::KeyUp:
                {
                    const auto* keyEvent = event.getIf<sf::Event::KeyReleased>();
                    if (e_itr.second == static_cast<int>(keyEvent->code))
                    {
                        if (bind->m_details.m_keyCode == -1) bind->m_details.m_keyCode = e_itr.second;
                        ++(bind->m_evCount);
                    }
                    break;
                }
                case EventType::MouseClick:
                {
                    const auto* mouseEvent = event.getIf<sf::Event::MouseButtonPressed>();
                    if (e_itr.second == static_cast<int>(mouseEvent->button))
                    {
                        bind->m_details.m_mouse.x = mouseEvent->position.x;
                        bind->m_details.m_mouse.y = mouseEvent->position.y;
                        if (bind->m_details.m_keyCode == -1) bind->m_details.m_keyCode = e_itr.second;
                        ++(bind->m_evCount);
                    }
                    break;
                }
                case EventType::MouseRelease:
                {
                    const auto* mouseEvent = event.getIf<sf::Event::MouseButtonReleased>();
                    if (e_itr.second == static_cast<int>(mouseEvent->button))
                    {
                        bind->m_details.m_mouse.x = mouseEvent->position.x;
                        bind->m_details.m_mouse.y = mouseEvent->position.y;
                        if (bind->m_details.m_keyCode == -1) bind->m_details.m_keyCode = e_itr.second;
                        ++(bind->m_evCount);
                    }
                    break;
                }
                case EventType::MouseWheel:
                {
                    const auto* mouseEvent = event.getIf<sf::Event::MouseWheelScrolled>();
                    if (e_itr.second == static_cast<int>(mouseEvent->wheel))
                    {
                        bind->m_details.m_mouse.x = mouseEvent->position.x;
                        bind->m_details.m_mouse.y = mouseEvent->position.y;
                        bind->m_details.m_mouseWheelDelta = mouseEvent->delta;
                        if (bind->m_details.m_keyCode == -1) bind->m_details.m_keyCode = e_itr.second;
                        ++(bind->m_evCount);
                    }
                    break;
                }
                case EventType::Resized:
                {
                    const auto* windowEvent = event.getIf<sf::Event::Resized>();
                    bind->m_details.m_size.x = windowEvent->size.x;
                    bind->m_details.m_size.y = windowEvent->size.y;
                    if (bind->m_details.m_keyCode == -1) bind->m_details.m_keyCode = e_itr.second;
                    ++(bind->m_evCount);

                    break;
                }
                case EventType::TextEntered:
                {
                    const auto* textEvent = event.getIf<sf::Event::TextEntered>();
                    bind->m_details.m_textEntered = textEvent->unicode;
                    if (bind->m_details.m_keyCode == -1) bind->m_details.m_keyCode = e_itr.second;
                    ++(bind->m_evCount);

                    break;
                }
                case EventType::Closed:
                    ++(bind->m_evCount);
                    break;

                default:
                    ++(bind->m_evCount);
            }
        }
    }
}

void EventManager::ProcessRealTimeInput()
{
    if (!m_hasFocus) return;

    for (auto& b_itr : m_bindings)          // Iterate through the bindings
    {
        Binding* bind = b_itr.second.get();
        for (auto& e_itr : bind->m_events)  // Iterate through the binding events
        {
            switch (e_itr.first)
            {
            case EventType::Keyboard:
            case EventType::KeyboardHeld:
            {
                if (sf::Keyboard::isKeyPressed(static_cast<sf::Keyboard::Key>(e_itr.second)))
                {
                    if (bind->m_details.m_keyCode == -1) bind->m_details.m_keyCode = e_itr.second;
                    ++(bind->m_evCount);
                }
                break;
            }
            case EventType::Mouse:
            case EventType::MouseHeld:
            {
                if (sf::Mouse::isButtonPressed(static_cast<sf::Mouse::Button>(e_itr.second)))
                {
                    if (bind->m_details.m_keyCode == -1) bind->m_details.m_keyCode = e_itr.second;
                    ++(bind->m_evCount);
                }
                break;
            }
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