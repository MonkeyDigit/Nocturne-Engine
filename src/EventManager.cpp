#include <optional>
#include "EventManager.h"
#include "StateManager.h"

// --- TRANSLATION DICTIONARIES ---
// Using an anonymous namespace to confine this data in this file only
// It gets allocated in memory only once on startup
namespace
{
    std::optional<EventType> ResolvePolledEventType(const sf::Event& event)
    {
        if (event.is<sf::Event::Closed>()) return EventType::Closed;
        if (event.is<sf::Event::Resized>()) return EventType::Resized;
        if (event.is<sf::Event::FocusLost>()) return EventType::FocusLost;
        if (event.is<sf::Event::FocusGained>()) return EventType::FocusGained;
        if (event.is<sf::Event::TextEntered>()) return EventType::TextEntered;
        if (event.is<sf::Event::KeyPressed>()) return EventType::KeyDown;
        if (event.is<sf::Event::KeyReleased>()) return EventType::KeyUp;
        if (event.is<sf::Event::MouseButtonPressed>()) return EventType::MouseClick;
        if (event.is<sf::Event::MouseButtonReleased>()) return EventType::MouseRelease;
        if (event.is<sf::Event::MouseWheelScrolled>()) return EventType::MouseWheel;
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