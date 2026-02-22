#include "Window.h"

Window::Window() { Setup("Project Nocturne", { 1280, 720 }); }

Window::Window(const std::string& title, const sf::Vector2u& size) {
    Setup(title, size);
}

Window::~Window() { Destroy(); }

void Window::Setup(const std::string& title, const sf::Vector2u& size) {
    m_windowTitle = title;
    m_windowSize = size;
    m_isFullscreen = false;
    m_isResizeable = true;
    m_isDone = false;
    m_isFocused = true; // Default
    m_frameRateLimit = 60;

    Create();
}

void Window::Create() {
    auto state = m_isFullscreen ? sf::State::Fullscreen : sf::State::Windowed;

    // Check if we should use fullscreen or windowed state
    m_window.create(sf::VideoMode(m_windowSize), m_windowTitle, state);
    m_window.setFramerateLimit(m_frameRateLimit);
}

void Window::Destroy() { m_window.close(); }

void Window::Update() {
    // In SFML 3, pollEvent returns std::optional<sf::Event>
    while (const std::optional<sf::Event> event = m_window.pollEvent()) {
        if (event->is<sf::Event::Closed>())
        {
            m_isDone = true;
        }
        else if (event->is<sf::Event::FocusLost>())
        {
            m_isFocused = false;
            m_eventManager.SetFocus(false);
        }
        else if (event->is<sf::Event::FocusGained>())
        {
            m_isFocused = true;
            m_eventManager.SetFocus(true);
        }
        else if (event->is<sf::Event::Resized>())
        {
            m_windowSize = m_window.getSize();
        }

        m_eventManager.HandleEvent(*event);
    }
    // TODO: Cosa fare di eventmanager set focus?
    m_eventManager.HandleUserInput();
    m_eventManager.Update();
}

void Window::SetResizeable(const bool resizeable)
{
    if (m_isResizeable == resizeable)
        return;

    m_isResizeable = resizeable;

    Destroy();
    Create();
}

void Window::SetFramerateLimit(const int limit)
{
    if (limit < 0)
    {
        std::cerr << "! Invalid frame rate limit: " << limit << '\n';
        return;
    }

    m_frameRateLimit = limit;
    m_window.setFramerateLimit(m_frameRateLimit);
}

void Window::ToggleFullscreen(EventDetails& details)
{
    if (details.m_heldDown)
        return;

    m_isFullscreen = !m_isFullscreen;
    Destroy(); // Destroy the current window
    Create();  // Recreate it with the new style
}

void Window::Close(EventDetails& details) { m_isDone = true; }
void Window::BeginDraw() { m_window.clear(sf::Color::Black); }
void Window::EndDraw() { m_window.display(); }
void Window::Draw(const sf::Drawable& drawable) { m_window.draw(drawable); }

sf::RenderWindow& Window::GetRenderWindow() { return m_window; }
EventManager& Window::GetEventManager() { return m_eventManager; }
sf::Vector2u Window::GetWindowSize() const { return m_windowSize; }
bool Window::IsFocused() const { return m_isFocused; }
bool Window::IsDone() const { return m_isDone; }
bool Window::IsFullscreen() const { return m_isFullscreen; }

sf::FloatRect Window::GetViewSpace() const {
    sf::Vector2f viewCenter = m_window.getView().getCenter();
    sf::Vector2f viewSize = m_window.getView().getSize();
    // SFML 3 style vector calculations
    return sf::FloatRect({ viewCenter.x - viewSize.x / 2.0f, viewCenter.y - viewSize.y / 2.0f }, viewSize);
}