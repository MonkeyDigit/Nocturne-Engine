#include <optional>
#include "Window.h"
#include "EngineLog.h"

Window::Window() { Setup("Project Nocturne", { 1280, 720 }); }

Window::Window(const std::string& title, const sf::Vector2u& size)
{
    Setup(title, size);
}

Window::~Window()
{
    Destroy();
}

void Window::Setup(const std::string& title, const sf::Vector2u& size)
{
    m_windowTitle = title;
    m_windowSize = size;
    m_isFullscreen = false;
    m_isResizeable = true;
    m_isDone = false;
    m_isFocused = true;
    m_frameRateLimit = 60;

    m_hasFixedAISeed = false;
    m_fixedAISeed = 0;

    LoadConfig();
    Create();
}

void Window::Create()
{
    const auto state = m_isFullscreen ? sf::State::Fullscreen : sf::State::Windowed;
    const unsigned int style = m_isResizeable
        ? sf::Style::Default
        : (sf::Style::Titlebar | sf::Style::Close);

    const sf::VideoMode mode = m_isFullscreen
        ? sf::VideoMode::getDesktopMode()
        : sf::VideoMode(m_windowSize);

    m_window.create(mode, m_windowTitle, style, state);

    m_windowSize = m_window.getSize();
    m_window.setFramerateLimit(m_frameRateLimit);
    m_window.setKeyRepeatEnabled(false);
}

sf::FloatRect Window::CalculateViewport(const sf::Vector2f& size) const
{
    if (m_windowSize.y == 0 || size.y == 0.0f) return sf::FloatRect();

    float windowRatio = static_cast<float>(m_windowSize.x) / static_cast<float>(m_windowSize.y);
    float targetRatio = size.x / size.y;

    float sizeX = 1.0f;
    float sizeY = 1.0f;
    float posX = 0.0f;
    float posY = 0.0f;

    if (windowRatio > targetRatio)
    {
        sizeX = targetRatio / windowRatio;
        posX = (1.0f - sizeX) * 0.5f;
    }
    else
    {
        sizeY = windowRatio / targetRatio;
        posY = (1.0f - sizeY) * 0.5f;
    }

    return sf::FloatRect({ posX, posY }, { sizeX, sizeY });
}

void Window::Destroy() { m_window.close(); }

void Window::Update()
{
    while (const std::optional<sf::Event> event = m_window.pollEvent())
    {
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

        m_eventManager.ProcessPolledEvent(*event);
    }

    m_eventManager.ProcessRealTimeInput();
    m_eventManager.DispatchCallbacks();
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
        EngineLog::Warn("Invalid frame rate limit: " + std::to_string(limit));
        return;
    }

    m_frameRateLimit = limit;
    m_window.setFramerateLimit(m_frameRateLimit);
}

void Window::ToggleFullscreen(EventDetails&)
{
    m_isFullscreen = !m_isFullscreen;
    Destroy();
    Create();
}

void Window::Close(EventDetails&) { m_isDone = true; }

void Window::BeginDraw() { m_window.clear(sf::Color::Black); }
void Window::EndDraw() { m_window.display(); }
void Window::Draw(const sf::Drawable& drawable) { m_window.draw(drawable); }

sf::RenderWindow& Window::GetRenderWindow() { return m_window; }
EventManager& Window::GetEventManager() { return m_eventManager; }
sf::Vector2u Window::GetWindowSize() const { return m_windowSize; }
bool Window::IsFocused() const { return m_isFocused; }
bool Window::IsDone() const { return m_isDone; }
bool Window::IsFullscreen() const { return m_isFullscreen; }

sf::View Window::GetGameView() const
{
    sf::View view(sf::FloatRect({ 0.0f, 0.0f }, m_gameResolution));
    view.setViewport(CalculateViewport(m_gameResolution));
    return view;
}

sf::View Window::GetUIView() const
{
    sf::View view(sf::FloatRect({ 0.0f, 0.0f }, m_uiResolution));
    view.setViewport(CalculateViewport(m_uiResolution));
    return view;
}

sf::FloatRect Window::GetViewSpace() const
{
    sf::Vector2f viewCenter = m_window.getView().getCenter();
    sf::Vector2f viewSize = m_window.getView().getSize();
    return sf::FloatRect({ viewCenter.x - viewSize.x / 2.0f, viewCenter.y - viewSize.y / 2.0f }, viewSize);
}