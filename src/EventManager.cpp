#include "EventManager.h"

EventManager::EventManager()
{
    // TODO: Load bindings from file
    // Setup default control scheme
    AddBinding("MoveLeft", sf::Keyboard::Key::Left);
    AddBinding("MoveRight", sf::Keyboard::Key::Right);
    AddBinding("Jump", sf::Keyboard::Key::Z);
    AddBinding("Attack", sf::Keyboard::Key::X);
}

void EventManager::AddBinding(const std::string& name, sf::Keyboard::Key key)
{
    m_bindings[name] = key;
    m_currentState[name] = false;
    m_previousState[name] = false;
}

void EventManager::Update()
{
    for (auto& pair : m_bindings)
    {
        const std::string& name = pair.first;
        sf::Keyboard::Key key = pair.second;

        // Shift current state to previous state
        m_previousState[name] = m_currentState[name];

        // Read new state directly from OS
        m_currentState[name] = sf::Keyboard::isKeyPressed(key);
    }
}

bool EventManager::IsActionHeld(const std::string& name) const
{
    auto it = m_currentState.find(name);
    if (it != m_currentState.end()) return it->second;
    return false;
}

bool EventManager::IsActionJustPressed(const std::string& name) const
{
    auto curr = m_currentState.find(name);
    auto prev = m_previousState.find(name);

    // True if pressed NOW, but NOT pressed in the previous frame
    if (curr != m_currentState.end() && prev != m_previousState.end())
    {
        return curr->second && !prev->second;
    }
    return false;
}

bool EventManager::IsActionJustReleased(const std::string& name) const
{
    auto curr = m_currentState.find(name);
    auto prev = m_previousState.find(name);

    // True if NOT pressed NOW, but WAS pressed in the previous frame
    if (curr != m_currentState.end() && prev != m_previousState.end())
    {
        return !curr->second && prev->second;
    }
    return false;
}