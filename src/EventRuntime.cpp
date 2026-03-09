#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

#include "EventManager.h"
#include "StateManager.h"

namespace
{
    using BindingMatchFlags = std::vector<std::uint8_t>;
    using MatchStateByBinding = std::unordered_map<const Binding*, BindingMatchFlags>;

    // Per-frame match state: cleared in DispatchCallbacks
    MatchStateByBinding g_matchState;

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

    BindingMatchFlags& EnsureMatchFlags(const Binding& bind)
    {
        BindingMatchFlags& flags = g_matchState[&bind];
        if (flags.size() != bind.m_events.size())
            flags.assign(bind.m_events.size(), 0u);
        return flags;
    }

    void MarkBindingMatched(Binding& bind, std::size_t eventIndex, int keyCode)
    {
        if (bind.m_events.empty()) return;

        BindingMatchFlags& flags = EnsureMatchFlags(bind);
        if (eventIndex >= flags.size()) return;

        // Count each required event once per frame, even if repeated in queue
        if (flags[eventIndex] != 0u) return;
        flags[eventIndex] = 1u;

        // Store the first matched key/button code for callback payload
        if (bind.m_details.m_keyCode == -1)
            bind.m_details.m_keyCode = keyCode;

        ++bind.m_evCount;
    }

    bool IsBindingFullyMatched(const Binding& bind)
    {
        if (bind.m_events.empty()) return false;
        if (bind.m_evCount != static_cast<int>(bind.m_events.size())) return false;

        auto it = g_matchState.find(&bind);
        if (it == g_matchState.end()) return false;

        for (std::uint8_t flag : it->second)
        {
            if (flag == 0u) return false;
        }

        return true;
    }
}

void EventManager::ProcessPolledEvent(const sf::Event& event)
{
    const std::optional<EventType> currentType = ResolvePolledEventType(event);
    if (!currentType) return;

    for (auto& [_, bindPtr] : m_bindings)
    {
        if (!bindPtr) continue;
        Binding& bind = *bindPtr;
        if (bind.m_events.empty()) continue;

        EnsureMatchFlags(bind);

        for (std::size_t eventIndex = 0; eventIndex < bind.m_events.size(); ++eventIndex)
        {
            const auto& [eventType, code] = bind.m_events[eventIndex];
            if (eventType != *currentType) continue;

            switch (eventType)
            {
            case EventType::KeyDown:
            {
                const auto* keyEvent = event.getIf<sf::Event::KeyPressed>();
                if (keyEvent && code == static_cast<int>(keyEvent->code))
                    MarkBindingMatched(bind, eventIndex, code);
                break;
            }
            case EventType::KeyUp:
            {
                const auto* keyEvent = event.getIf<sf::Event::KeyReleased>();
                if (keyEvent && code == static_cast<int>(keyEvent->code))
                    MarkBindingMatched(bind, eventIndex, code);
                break;
            }
            case EventType::MouseClick:
            {
                const auto* mouseEvent = event.getIf<sf::Event::MouseButtonPressed>();
                if (mouseEvent && code == static_cast<int>(mouseEvent->button))
                {
                    bind.m_details.m_mouse.x = mouseEvent->position.x;
                    bind.m_details.m_mouse.y = mouseEvent->position.y;
                    MarkBindingMatched(bind, eventIndex, code);
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
                    MarkBindingMatched(bind, eventIndex, code);
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
                    MarkBindingMatched(bind, eventIndex, code);
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
                    MarkBindingMatched(bind, eventIndex, code);
                }
                break;
            }
            case EventType::TextEntered:
            {
                const auto* textEvent = event.getIf<sf::Event::TextEntered>();
                if (textEvent)
                {
                    bind.m_details.m_textEntered = textEvent->unicode;
                    MarkBindingMatched(bind, eventIndex, code);
                }
                break;
            }
            case EventType::Closed:
            case EventType::FocusLost:
            case EventType::FocusGained:
                MarkBindingMatched(bind, eventIndex, code);
                break;

            default:
                // Type already matched by ResolvePolledEventType
                MarkBindingMatched(bind, eventIndex, code);
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
        if (!bindPtr) continue;
        Binding& bind = *bindPtr;
        if (bind.m_events.empty()) continue;

        EnsureMatchFlags(bind);

        for (std::size_t eventIndex = 0; eventIndex < bind.m_events.size(); ++eventIndex)
        {
            const auto& [eventType, code] = bind.m_events[eventIndex];
            if (code < 0) continue;

            switch (eventType)
            {
            case EventType::Keyboard:
            case EventType::KeyboardHeld:
                if (sf::Keyboard::isKeyPressed(static_cast<sf::Keyboard::Key>(code)))
                    MarkBindingMatched(bind, eventIndex, code);
                break;

            case EventType::Mouse:
            case EventType::MouseHeld:
                if (sf::Mouse::isButtonPressed(static_cast<sf::Mouse::Button>(code)))
                    MarkBindingMatched(bind, eventIndex, code);
                break;

            default:
                break;
            }
        }
    }
}

// If all events in a binding matched, dispatch its callback
void EventManager::DispatchCallbacks()
{
    for (auto& [_, bindPtr] : m_bindings)
    {
        if (!bindPtr) continue;
        Binding& bind = *bindPtr;

        if (IsBindingFullyMatched(bind))
        {
            auto dispatchForState = [&](StateType state)
                {
                    auto callbacksIt = m_callbacks.find(state);
                    if (callbacksIt == m_callbacks.end()) return;

                    auto callItr = callbacksIt->second.find(bind.m_name);
                    if (callItr != callbacksIt->second.end())
                        callItr->second(bind.m_details);
                };

            dispatchForState(m_currentState);

            // Also dispatch global callbacks, but avoid duplicate dispatch
            if (m_currentState != StateType::Global)
                dispatchForState(StateType::Global);
        }

        bind.m_evCount = 0;
        bind.m_details.Clear();
    }

    g_matchState.clear();
}