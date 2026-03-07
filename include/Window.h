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

    sf::View GetGameView() const;
    sf::View GetUIView() const;

    const sf::Vector2f& GetGameResolution() const { return m_gameResolution; }
    const sf::Vector2f& GetUIResolution() const { return m_uiResolution; }

private:
    void Setup(const std::string& title, const sf::Vector2u& size);
    void Destroy();
    void Create();
    void LoadConfig();
    sf::FloatRect CalculateViewport(const sf::Vector2f& size) const;

    sf::Vector2f m_gameResolution;
    sf::Vector2f m_uiResolution;
    sf::FloatRect m_viewport;
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