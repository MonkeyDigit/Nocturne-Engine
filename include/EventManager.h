#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <string>
#include "Utilities.h"

// --- EVENT TYPE STRING CONSTANTS ---
// We use strings to identify event types, making config files human-readable
namespace EventTypes
{
    //These are handled by HandleWindowEvent()
    const std::string Closed = "Closed";
    const std::string Resized = "Resized";
    const std::string FocusLost = "FocusLost";
    const std::string FocusGained = "FocusGained";
    const std::string TextEntered = "TextEntered";
    const std::string KeyDown = "KeyDown";
    const std::string KeyUp = "KeyUp";
    const std::string MouseWheel = "MouseWheel";
    const std::string MouseClick = "MouseClick";
    const std::string MouseRelease = "MouseRelease";
    const std::string KeyboardHeld = "KeyboardHeld"; // Real-time input // TODO: Togliere?
    const std::string MouseHeld = "MouseHeld";       // Real-time input // TODO: Togliere?
    //These are handled by HandleUserInput() and will be detected without delay. Use these for real time input
    // TODO: Ha senso mantenere questa distinzione ora che non usa più gli id?
    const std::string Keyboard = "Keyboard";
    const std::string Mouse = "Mouse";
    const std::string Joystick = "Joystick";
}

// --- EVENT INFO STRUCT ---
struct EventInfo
{
    EventInfo(int code = 0) : m_code(code) {}
    int m_code; // Can be a sf::Keyboard::Key or sf::Mouse::Button casted to int
};

//event type: button down; button up...
//event info: key of the button pressed; 0, 1, 2...
using Events = std::vector<std::pair<std::string, EventInfo>>;

// --- EVENT DETAILS STRUCT ---
struct EventDetails
{
    EventDetails(const std::string& bindName);
    void Clear();

    std::string m_name;         //Name of the binding
    sf::Vector2i m_size;        // Window size
    char32_t m_textEntered;
    sf::Vector2i m_mouse;
    int m_mouseWheelDelta;
    int m_keyCode;              //Single key code
    // TODO: Da implementare meglio?
    bool m_heldDown;            //If the key was held down instead of a first press at the time of detection
};

// --- BINDING STRUCT ---
struct Binding
{
    Binding(const std::string& bindName);
    void BindEvent(const std::string& type, EventInfo info = EventInfo());

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
        return itr->second.emplace(name, temp).second;
    }

    bool RemoveCallback(StateType state, const std::string& name);
    void SetCurrentState(StateType type);

    void HandleEvent(const sf::Event& event);   //Needed to retrieve information from the event
    void HandleUserInput();                     //Needed to handle user input without the delay of pollEvent()
    void Update();

private:
    void LoadBindings(const std::string& path);
    // Helper to convert strings like "Left" to sf::Keyboard::Key::Left
    int ParseKeyCode(const std::string& keyStr);
    int ParseMouseButton(const std::string& btnStr);

    StateType m_currentState;
    Bindings m_bindings;
    Callbacks m_callbacks;
    bool m_hasFocus;
};