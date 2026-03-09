#include "EventManager.h"
#include "StateManager.h"

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

// --- EVENT MANAGER CORE ---
EventManager::EventManager()
    : m_currentState(StateType::Global),
    m_hasFocus(true)
{
    LoadBindings("config/bindings.cfg");
}

bool EventManager::AddBinding(std::unique_ptr<Binding> binding)
{
    if (!binding) return false;
    if (binding->m_name.empty()) return false;
    if (binding->m_events.empty()) return false; // Prevent always-true callbacks.
    if (m_bindings.find(binding->m_name) != m_bindings.end()) return false;

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