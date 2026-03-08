#include <fstream>
#include <sstream>
#include <iostream>
#include "Window.h"

Window::Window() { Setup("Project Nocturne", { 1280, 720 }); }

Window::Window(const std::string& title, const sf::Vector2u& size)
{
    Setup(title, size);
}

Window::~Window() 
{
    // TODO: Remove callback ???
    Destroy();
}

void Window::Setup(const std::string& title, const sf::Vector2u& size)
{
    m_windowTitle = title;
    m_windowSize = size;
    m_isFullscreen = false;
    m_isResizeable = true;
    m_isDone = false;
    m_isFocused = true; // Default
    m_frameRateLimit = 60;

    LoadConfig();

    Create();
    // TODO: Add callback functions ???
}

void Window::Create()
{
    auto state = m_isFullscreen ? sf::State::Fullscreen : sf::State::Windowed;

    // Check if we should use fullscreen or windowed state
    m_window.create(sf::VideoMode(m_windowSize), m_windowTitle, state);
    m_window.setFramerateLimit(m_frameRateLimit);
    m_window.setKeyRepeatEnabled(false);
    // TODO: Gestire le varie clausole di isfullscreen, isresizeable ecc...
}

void Window::LoadConfig()
{
    // Default values
    m_gameResolution = { 640.0f, 360.0f };
    m_uiResolution = { 1280.0f, 720.0f };

    std::ifstream file(Utils::GetWorkingDirectory() + "config/window.cfg");
    if (!file.is_open())
    {
        std::cerr << "! Failed to load window.cfg. Using defaults.\n";
        return;
    }

    std::string line;
    unsigned int lineNumber = 0;

    while (std::getline(file, line))
    {
        ++lineNumber;

        // Skip empty or comment lines
        if (line.empty() || line[0] == '|' || line[0] == '#') continue;

        std::stringstream keystream(line);
        std::string type;
        if (!(keystream >> type)) continue;

        if (type == "GameRes")
        {
            float x = 0.0f, y = 0.0f;
            if (!(keystream >> x >> y) || x <= 0.0f || y <= 0.0f)
            {
                std::cerr << "! Invalid GameRes at line " << lineNumber << '\n';
                continue;
            }
            m_gameResolution = { x, y };
        }
        else if (type == "UIRes")
        {
            float x = 0.0f, y = 0.0f;
            if (!(keystream >> x >> y) || x <= 0.0f || y <= 0.0f)
            {
                std::cerr << "! Invalid UIRes at line " << lineNumber << '\n';
                continue;
            }
            m_uiResolution = { x, y };
        }
        else
        {
            std::cerr << "! Unknown window config key '" << type
                << "' at line " << lineNumber << '\n';
        }
    }

    // RAII closes automatically
}


sf::FloatRect Window::CalculateViewport(const sf::Vector2f& size) const
{
    // Avoid division by zero
    if (m_windowSize.y == 0 || size.y == 0.0f) return sf::FloatRect();

    float windowRatio = static_cast<float>(m_windowSize.x) / static_cast<float>(m_windowSize.y);
    float targetRatio = size.x / size.y;

    float sizeX = 1.0f;
    float sizeY = 1.0f;
    float posX = 0.0f;
    float posY = 0.0f;

    // Compare aspect ratio
    if (windowRatio > targetRatio)
    {
        // Window too large
        sizeX = targetRatio / windowRatio;
        posX = (1.0f - sizeX) * 0.5f;
    }
    else
    {
        // Window too long
        sizeY = windowRatio / targetRatio;
        posY = (1.0f - sizeY) * 0.5f;
    }

    return sf::FloatRect({ posX, posY }, { sizeX, sizeY });
}

void Window::Destroy() { m_window.close(); }

void Window::Update()
{
    // In SFML 3, pollEvent returns std::optional<sf::Event>
    // TODO: Singolo = ???
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

        m_eventManager.ProcessPolledEvent(*event);
    }
    // TODO: Cosa fare di eventmanager set focus?
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
        std::cerr << "! Invalid frame rate limit: " << limit << '\n';
        return;
    }

    m_frameRateLimit = limit;
    m_window.setFramerateLimit(m_frameRateLimit);
}

void Window::ToggleFullscreen(EventDetails& details)
{
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
    // SFML 3 style vector calculations
    return sf::FloatRect({ viewCenter.x - viewSize.x / 2.0f, viewCenter.y - viewSize.y / 2.0f }, viewSize);
}