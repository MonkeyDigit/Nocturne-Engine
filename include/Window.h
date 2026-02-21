#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>
#include <string>

class Window
{
public:
    Window();
    Window(const std::string& title, const sf::Vector2u& size);
    ~Window();

    void BeginDraw(); // Clears the window
    void EndDraw();   // Displays the changes

    void Update();    // Handles OS events (like closing the window)

    bool IsDone() const;
    bool IsFullscreen() const;
    sf::Vector2u GetWindowSize() const;

    void ToggleFullscreen();

    // Helper to draw drawables directly through the wrapper
    void Draw(const sf::Drawable& drawable);

    // We expose the raw RenderWindow by reference for things like sf::View (Camera)
    sf::RenderWindow& GetRenderWindow();

private:
    void Setup(const std::string& title, const sf::Vector2u& size);
    void Destroy();
    void Create();

    sf::RenderWindow m_window;
    sf::Vector2u m_windowSize;
    std::string m_windowTitle;
    bool m_isDone;
    bool m_isFullscreen;
    // TODO: isFocused, isresizeable
};