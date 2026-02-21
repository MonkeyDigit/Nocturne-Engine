#pragma once
#include <SFML/Window/Keyboard.hpp>
#include <unordered_map>
#include <string>

class EventManager
{
public:
    EventManager();

    // Bind an action name to a specific key
    void AddBinding(const std::string& name, sf::Keyboard::Key key);

    // Called once per frame to update current and previous key states
    void Update();

    // Input Queries
    bool IsActionHeld(const std::string& name) const;
    bool IsActionJustPressed(const std::string& name) const;
    bool IsActionJustReleased(const std::string& name) const;

private:
    std::unordered_map<std::string, sf::Keyboard::Key> m_bindings;
    std::unordered_map<std::string, bool> m_currentState;
    std::unordered_map<std::string, bool> m_previousState;
};