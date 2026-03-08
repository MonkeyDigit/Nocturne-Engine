#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <string>
#include "Utilities.h"

enum class EventType
{
    Closed, Resized, FocusLost, FocusGained, TextEntered,
    KeyDown, KeyUp, MouseWheel, MouseClick, MouseRelease,
    KeyboardHeld, MouseHeld, Keyboard, Mouse, Joystick
};

//event type: button down; button up...
//event info: key of the button pressed; 0, 1, 2...
using Events = std::vector<std::pair<EventType, int>>;

// --- EVENT DETAILS STRUCT ---
struct EventDetails
{
    EventDetails(const std::string& bindName);
    void Clear();

    std::string m_name;         //Name of the binding
    sf::Vector2i m_size;        // Window size
    char32_t m_textEntered;
    sf::Vector2i m_mouse;
    float m_mouseWheelDelta;
    int m_keyCode;              //Single key code
};

// --- BINDING STRUCT ---
struct Binding
{
    Binding(const std::string& bindName);
    void BindEvent(EventType type, int code = 0);

    Events m_events;
    std::string m_name;
    int m_evCount; // Count of events that are currently "happening"
    EventDetails m_details;
};
// Using an unordered map guarantees that there will be only 1 binding per action
using Bindings = std::unordered_map<std::string, std::unique_ptr<Binding>>;
// std function wrapper
using CallbackContainer = std::unordered_map<std::string, std::function<void(EventDetails&)>>;
// Forward declaration (will be defined in StateManager)
enum class StateType;
using Callbacks = std::unordered_map<StateType, CallbackContainer>;

// --- EVENT MANAGER CLASS ---
class EventManager
{
public:
    EventManager();
    ~EventManager() = default;

    bool AddBinding(std::unique_ptr<Binding> binding);
    bool RemoveBinding(const std::string& name);
    void SetFocus(bool focus);

    sf::Vector2i GetMousePos() const;
    sf::Vector2i GetMousePos(const sf::RenderWindow& window) const;

    // Needs to be defined in the header!
    template<class T>
bool AddCallback(StateType state, const std::string& name,
    void(T::* func)(EventDetails&), T& instance)
{
    auto itr = m_callbacks.emplace(state, CallbackContainer()).first;
    auto temp = std::bind(func, &instance, std::placeholders::_1);

    // Replace existing callback with same name to avoid stale handlers
    itr->second[name] = std::move(temp);
    return true;
}

    bool RemoveCallback(StateType state, const std::string& name);
    void SetCurrentState(StateType type);

    void ProcessPolledEvent(const sf::Event& event);   // For discrete input
    void ProcessRealTimeInput();                     // For continuous input
    void DispatchCallbacks();

private:
    void LoadBindings(const std::string& path);
    // Helper to convert strings like "Left" to sf::Keyboard::Key::Left
    int ParseEventInfo(EventType evtype, const std::string& evinfoStr);

    StateType m_currentState;
    Bindings m_bindings;
    Callbacks m_callbacks;
    bool m_hasFocus;
};