#pragma once
#include <SFML/Graphics.hpp>
#include "EventManager.h"
#include <string>

class Window {
public:
    Window();
    Window(const std::string& title, const sf::Vector2u& size);
    ~Window();

    void BeginDraw();                           // Clears the window
    void Draw(const sf::Drawable& drawable);    // Draw drawables directly through the wrapper
    void EndDraw();                             // Displays the changes

    void Update();    // Handles OS events (like closing the window)
    void SetResizeable(const bool resizeable);
    void SetFramerateLimit(const int limit);

    void Close(EventDetails& details);
    void ToggleFullscreen(EventDetails& details);

    // We expose the raw RenderWindow by reference for things like sf::View (Camera)
    // We use a reference because we don't forsee the window to be a nullptr
    sf::RenderWindow& GetRenderWindow();
    EventManager& GetEventManager();
    sf::Vector2u GetWindowSize() const;
    sf::FloatRect GetViewSpace() const;
    bool IsFocused() const;
    bool IsDone() const;
    bool IsFullscreen() const;

private:
    void Setup(const std::string& title, const sf::Vector2u& size);
    void Destroy();
    void Create();

    sf::RenderWindow m_window;
    sf::Vector2u m_windowSize;
    std::string m_windowTitle;
    EventManager m_eventManager;

    bool m_isFocused;
    bool m_isDone;
    bool m_isFullscreen;
    bool m_isResizeable;
    int m_frameRateLimit;
};