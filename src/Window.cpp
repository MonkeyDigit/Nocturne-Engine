#include "Window.h"

Window::Window()
{
    Setup("Window", { 640, 480 });
}

Window::Window(const std::string& title, const sf::Vector2u& size)
{
    Setup(title, size);
}

Window::~Window()
{
    // TODO: EVENT MANAGER
    Destroy();
}

void Window::Setup(const std::string& title, const sf::Vector2u& size)
{
    m_windowTitle = title;
    m_windowSize = size;
    m_isFullscreen = false;
    m_isDone = false;
    Create();
}

void Window::Create()
{
    // TODO: SF view
    // TODO: FRAMERATE CAP
    // Check if we should use fullscreen or windowed state
    auto state = m_isFullscreen ? sf::State::Fullscreen : sf::State::Windowed;

    m_window.create(sf::VideoMode(m_windowSize), m_windowTitle, state);
    m_window.setVerticalSyncEnabled(false);
}

void Window::Destroy()
{
    m_window.close();
}

void Window::Update()
{
    // In SFML 3, pollEvent returns std::optional<sf::Event>
    while (const std::optional<sf::Event> event = m_window.pollEvent())
    {
        // TODO: isFocused, resized
        // Check if window was closed
        if (event->is<sf::Event::Closed>())
        {
            m_isDone = true;
        }
        // Handle Fullscreen toggle with F5 key
        else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
        {
            if (keyPressed->code == sf::Keyboard::Key::F5)
            {
                ToggleFullscreen();
            }
        }

        // TODO: handle window events, update events
    }
}

void Window::ToggleFullscreen()
{
    // TODO: Held down case
    m_isFullscreen = !m_isFullscreen;
    Destroy(); // Destroy the current window
    Create();  // Recreate it with the new style
}

void Window::BeginDraw()
{
    m_window.clear(sf::Color::Black);
}

void Window::EndDraw()
{
    m_window.display();
}

bool Window::IsDone() const { return m_isDone; }
bool Window::IsFullscreen() const { return m_isFullscreen; }
sf::Vector2u Window::GetWindowSize() const { return m_windowSize; }
sf::RenderWindow& Window::GetRenderWindow() { return m_window; }
void Window::Draw(const sf::Drawable& drawable) { m_window.draw(drawable); }