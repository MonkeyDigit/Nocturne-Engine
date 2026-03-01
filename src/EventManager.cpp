#include "EventManager.h"
#include <iostream>
#include <fstream>
#include <sstream>

// TODO: SOSTITUIRE STA ROBA? RIMETTERE IL NAMESPACE?
// --- TRANSLATION DICTIONARIES ---
// Using an anonymous namespace to confine this data in this file only
// It gets allocated in memory only once on startup
namespace
{
    const std::unordered_map<std::string, sf::Keyboard::Key> KEY_MAP = {
        {"A", sf::Keyboard::Key::A}, {"B", sf::Keyboard::Key::B}, {"C", sf::Keyboard::Key::C},
        {"D", sf::Keyboard::Key::D}, {"E", sf::Keyboard::Key::E}, {"F", sf::Keyboard::Key::F},
        {"G", sf::Keyboard::Key::G}, {"H", sf::Keyboard::Key::H}, {"I", sf::Keyboard::Key::I},
        {"J", sf::Keyboard::Key::J}, {"K", sf::Keyboard::Key::K}, {"L", sf::Keyboard::Key::L},
        {"M", sf::Keyboard::Key::M}, {"N", sf::Keyboard::Key::N}, {"O", sf::Keyboard::Key::O},
        {"P", sf::Keyboard::Key::P}, {"Q", sf::Keyboard::Key::Q}, {"R", sf::Keyboard::Key::R},
        {"S", sf::Keyboard::Key::S}, {"T", sf::Keyboard::Key::T}, {"U", sf::Keyboard::Key::U},
        {"V", sf::Keyboard::Key::V}, {"W", sf::Keyboard::Key::W}, {"X", sf::Keyboard::Key::X},
        {"Y", sf::Keyboard::Key::Y}, {"Z", sf::Keyboard::Key::Z},
        {"Left", sf::Keyboard::Key::Left}, {"Right", sf::Keyboard::Key::Right},
        {"Up", sf::Keyboard::Key::Up}, {"Down", sf::Keyboard::Key::Down},
        {"Space", sf::Keyboard::Key::Space}, {"Enter", sf::Keyboard::Key::Enter},
        {"Escape", sf::Keyboard::Key::Escape}, {"LShift", sf::Keyboard::Key::LShift}
        // TODO: AGGIUNGERE F1 F2 ecc
    };

    const std::unordered_map<std::string, sf::Mouse::Button> MOUSE_MAP = {
        {"Left", sf::Mouse::Button::Left},
        {"Right", sf::Mouse::Button::Right},
        {"Middle", sf::Mouse::Button::Middle}
    };
}

// --- EVENT DETAILS ---
EventDetails::EventDetails(const std::string& bindName) : m_name(bindName) { Clear(); }

void EventDetails::Clear()
{
    m_size = sf::Vector2i(0, 0);
    m_textEntered = 0;
    m_mouse = sf::Vector2i(0, 0);
    m_mouseWheelDelta = 0;
    m_keyCode = -1;
}

// --- BINDING ---
Binding::Binding(const std::string& bindName) : m_name(bindName), m_details(bindName), m_evCount(0) {}

void Binding::BindEvent(const std::string& type, EventInfo info)
{
    m_events.emplace_back(type, info);
}

// --- EVENT MANAGER ---
EventManager::EventManager() : m_hasFocus(true)
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
// --- PARSER UNIFICATO ---
int EventManager::ParseEventInfo(const std::string& evtype, const std::string& evinfoStr)
{
    // KEYBOARD EVENT
    if (evtype == EventTypes::KeyDown || evtype == EventTypes::KeyUp ||
        evtype == EventTypes::KeyboardHeld || evtype == EventTypes::Keyboard)
    {
        auto it = KEY_MAP.find(evinfoStr);
        if (it != KEY_MAP.end()) return static_cast<int>(it->second);
    }
    // MOUSE EVENT
    else if (evtype == EventTypes::MouseClick || evtype == EventTypes::MouseRelease ||
        evtype == EventTypes::MouseHeld || evtype == EventTypes::Mouse)
    {
        auto it = MOUSE_MAP.find(evinfoStr);
        if (it != MOUSE_MAP.end()) return static_cast<int>(it->second);
    }

    // Fallback to int value
    try {
        return std::stoi(evinfoStr);
    }
    catch (...) {
        std::cerr << "! Error (stoi): impossible conversion '" << evinfoStr << "' of event " << evtype << '\n';
        return -1;
    }
}

void EventManager::LoadBindings(const std::string& path)
{
    std::ifstream bindings{ path };
    if (!bindings.is_open())
    {
        std::cerr << "! Failed loading bindings file: " << path << '\n';
        return;
    }

    std::string line;                       // Each line is a binding with the callback name and the events
    while (std::getline(bindings, line))
    {
        if (line.empty() || line[0] == '|') continue;

        // GET CALLBACK NAME
        std::stringstream keystream(line);
        std::string callbackName;
        keystream >> callbackName;

        auto bind = std::make_unique<Binding>(callbackName);

        // READ EVENTS
        while (!keystream.eof())    // Bind the events to the callback
        {
            std::string eventToken;
            keystream >> eventToken; // e.g. "KeyDown:Left" or "Closed:0"
            if (eventToken.empty()) break;

            size_t colonPos = eventToken.find(':');
            if (colonPos == std::string::npos) break; // Invalid format

            std::string evtype = eventToken.substr(0, colonPos);
            std::string evinfoStr = eventToken.substr(colonPos + 1);

            EventInfo info(ParseEventInfo(evtype, evinfoStr));
            bind->BindEvent(evtype, info);
        }


        if (!AddBinding(std::move(bind)))
            std::cerr << "! Couldn't load binding: " << callbackName << '\n';
    }

    bindings.close();
}

void EventManager::HandleEvent(const sf::Event& event)
{
    std::string currentSFMLEventType = "";
    if (event.is<sf::Event::KeyPressed>()) currentSFMLEventType = EventTypes::KeyDown;
    else if (event.is<sf::Event::KeyReleased>()) currentSFMLEventType = EventTypes::KeyUp;
    else if (event.is<sf::Event::MouseButtonPressed>()) currentSFMLEventType = EventTypes::MouseClick;
    else if (event.is<sf::Event::MouseButtonReleased>()) currentSFMLEventType = EventTypes::MouseRelease;
    else if (event.is<sf::Event::MouseWheelScrolled>()) currentSFMLEventType = EventTypes::MouseWheel;
    else if (event.is<sf::Event::Resized>()) currentSFMLEventType = EventTypes::Resized;
    else if (event.is<sf::Event::TextEntered>()) currentSFMLEventType = EventTypes::TextEntered;
    else if (event.is<sf::Event::Closed>()) currentSFMLEventType = EventTypes::Closed;

    if (currentSFMLEventType.empty()) return;

    for (auto& b_itr : m_bindings)              // Iterate through the bindings
    {
        Binding* bind = b_itr.second.get();
        for (auto& e_itr : bind->m_events)      // Iterate through the binding events
        {
            const std::string& bindType = e_itr.first;

            if (bindType != currentSFMLEventType) continue;

            if (bindType == EventTypes::KeyDown)
            {
                const auto* keyEvent = event.getIf<sf::Event::KeyPressed>();
                if (e_itr.second.m_code == static_cast<int>(keyEvent->code))
                {
                    // TODO: CI VA != ?
                    if (bind->m_details.m_keyCode == -1) bind->m_details.m_keyCode = e_itr.second.m_code;
                    ++(bind->m_evCount);
                }
            }
            else if (bindType == EventTypes::KeyUp)
            {
                const auto* keyEvent = event.getIf<sf::Event::KeyReleased>();
                if (e_itr.second.m_code == static_cast<int>(keyEvent->code))
                {
                    if (bind->m_details.m_keyCode == -1) bind->m_details.m_keyCode = e_itr.second.m_code;
                    ++(bind->m_evCount);
                }
            }
            else if (bindType == EventTypes::MouseClick)
            {
                const auto* mouseEvent = event.getIf<sf::Event::MouseButtonPressed>();
                if (e_itr.second.m_code == static_cast<int>(mouseEvent->button))
                {
                    bind->m_details.m_mouse.x = mouseEvent->position.x;
                    bind->m_details.m_mouse.y = mouseEvent->position.y;
                    if (bind->m_details.m_keyCode == -1) bind->m_details.m_keyCode = e_itr.second.m_code;
                    ++(bind->m_evCount);
                }
            }
            else if (bindType == EventTypes::MouseRelease)
            {
                const auto* mouseEvent = event.getIf<sf::Event::MouseButtonReleased>();
                if (e_itr.second.m_code == static_cast<int>(mouseEvent->button))
                {
                    bind->m_details.m_mouse.x = mouseEvent->position.x;
                    bind->m_details.m_mouse.y = mouseEvent->position.y;
                    if (bind->m_details.m_keyCode == -1) bind->m_details.m_keyCode = e_itr.second.m_code;
                    ++(bind->m_evCount);
                }
            }
            else if (bindType == EventTypes::MouseWheel)
            {
                const auto* mouseEvent = event.getIf<sf::Event::MouseWheelScrolled>();
                if (e_itr.second.m_code == static_cast<int>(mouseEvent->wheel))
                {
                    bind->m_details.m_mouse.x = mouseEvent->position.x;
                    bind->m_details.m_mouse.y = mouseEvent->position.y;
                    bind->m_details.m_mouseWheelDelta = mouseEvent->delta;
                    if (bind->m_details.m_keyCode == -1) bind->m_details.m_keyCode = e_itr.second.m_code;
                    ++(bind->m_evCount);
                }
            }
            else if (bindType == EventTypes::Resized)
            {
                const auto* windowEvent = event.getIf<sf::Event::Resized>();
                bind->m_details.m_size.x = windowEvent->size.x;
                bind->m_details.m_size.y = windowEvent->size.y;
                if (bind->m_details.m_keyCode == -1) bind->m_details.m_keyCode = e_itr.second.m_code;
                ++(bind->m_evCount);
            }
            else if (bindType == EventTypes::TextEntered)
            {
                const auto* textEvent = event.getIf<sf::Event::TextEntered>();
                bind->m_details.m_textEntered = textEvent->unicode;
                if (bind->m_details.m_keyCode == -1) bind->m_details.m_keyCode = e_itr.second.m_code;
                ++(bind->m_evCount);
            }
            else if (bindType == EventTypes::Closed)
            {
                ++(bind->m_evCount);
            }
            else
            {
                ++(bind->m_evCount);
            }
        }
    }
}

void EventManager::HandleUserInput()
{
    for (auto& b_itr : m_bindings)          // Iterate through the bindings
    {
        Binding* bind = b_itr.second.get();
        for (auto& e_itr : bind->m_events)  // Iterate through the binding events
        {
            if (e_itr.first == EventTypes::Keyboard)
            {
                if (sf::Keyboard::isKeyPressed(static_cast<sf::Keyboard::Key>(e_itr.second.m_code)))
                {
                    // TODO: != ?
                    if (bind->m_details.m_keyCode == -1) bind->m_details.m_keyCode = e_itr.second.m_code;
                    ++(bind->m_evCount);
                }
            }
            else if (e_itr.first == EventTypes::Mouse)
            {
                if (sf::Mouse::isButtonPressed(static_cast<sf::Mouse::Button>(e_itr.second.m_code)))
                {
                    // TODO: != ?
                    if (bind->m_details.m_keyCode == -1) bind->m_details.m_keyCode = e_itr.second.m_code;
                    ++(bind->m_evCount);
                }
            }
        }
    }
}

// At this point, if the matched events counter is equal to the number of the binding's triggering events, its callback function is called
void EventManager::Update()
{
    if (!m_hasFocus) return;

    for (auto& b_itr : m_bindings)  // Iterate through the bindings
    {
        Binding* bind = b_itr.second.get();
        if (bind->m_events.size() == bind->m_evCount)
        {
            auto stateCallbacks = m_callbacks.find(m_currentState);
            auto otherCallbacks = m_callbacks.find(static_cast<StateType>(0));

            if (stateCallbacks != m_callbacks.end())
            {
                auto callItr = stateCallbacks->second.find(bind->m_name);
                if (callItr != stateCallbacks->second.end())
                {   // Pass in information about events
                    callItr->second(bind->m_details);
                    // This is set to true after executing the binding once
                    // so that, the next time it is called, you can check if the binding was held down
                    bind->m_details.m_heldDown = true;
                }
            }

            // Also check global callbacks (State 0)
            if (otherCallbacks != m_callbacks.end())
            {
                auto callItr = otherCallbacks->second.find(bind->m_name);
                if (callItr != otherCallbacks->second.end())
                {   // Pass in information about events
                    callItr->second(bind->m_details);
                    bind->m_details.m_heldDown = true;
                }
            }
        }
        else
        {
            bind->m_details.m_heldDown = false;
        }

        bind->m_evCount = 0;
        bind->m_details.Clear();
    }
}