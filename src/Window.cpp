#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <limits>
#include "Window.h"
#include "EngineLog.h"
#include "ConfigParseUtils.h"

namespace
{
    using ParseUtils::TryReadExact;

    std::string ToLowerCopy(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return value;
    }

    bool TryParseLogLevel(const std::string& raw, EngineLog::Level& outLevel)
    {
        const std::string value = ToLowerCopy(raw);
        if (value == "info") { outLevel = EngineLog::Level::Info; return true; }
        if (value == "warn" || value == "warning") { outLevel = EngineLog::Level::Warn; return true; }
        if (value == "error") { outLevel = EngineLog::Level::Error; return true; }
        return false;
    }

    std::string CanonicalWindowKey(std::string key)
    {
        key = ToLowerCopy(std::move(key));
        if (key == "resizeable") return "resizable";
        if (key == "windowsize") return "windowres";
        return key;
    }

    void WarnWindowConfigLine(unsigned int lineNumber, const std::string& message)
    {
        EngineLog::Warn("window.cfg line " + std::to_string(lineNumber) + ": " + message);
    }
}

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
    m_isFocused = true; // Default
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

    // Keep cached size aligned with actual OS window size
    m_windowSize = m_window.getSize();

    m_window.setFramerateLimit(m_frameRateLimit);
    m_window.setKeyRepeatEnabled(false);
}

void Window::LoadConfig()
{
    // Engine-safe defaults
    m_gameResolution = { 640.0f, 360.0f };
    m_uiResolution = { 1280.0f, 720.0f };

    std::ifstream file(Utils::GetWorkingDirectory() + "config/window.cfg");
    if (!file.is_open())
    {
        EngineLog::WarnOnce("window.config.missing", "Failed to load window.cfg. Using defaults.");
        return;
    }

    std::unordered_set<std::string> seenKeys;
    std::string line;
    unsigned int lineNumber = 0;

    while (std::getline(file, line))
    {
        ++lineNumber;

        // Support inline comments and full-line comments
        const size_t commentPos = line.find('#');
        if (commentPos != std::string::npos)
            line.erase(commentPos);

        const size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) continue;
        if (line[first] == '|') continue;

        std::stringstream keystream(line);
        std::string rawType;
        if (!(keystream >> rawType)) continue;

        const std::string type = CanonicalWindowKey(rawType);

        if (!seenKeys.insert(type).second)
        {
            WarnWindowConfigLine(
                lineNumber,
                "Duplicate key '" + rawType + "'. Last valid value wins.");
        }

        if (type == "gameres")
        {
            float x = 0.0f, y = 0.0f;
            if (!TryReadExact(keystream, x, y) || x <= 0.0f || y <= 0.0f)
            {
                WarnWindowConfigLine(lineNumber, "Invalid GameRes (expected: GameRes <x> <y>, both > 0).");
                continue;
            }
            m_gameResolution = { x, y };
        }
        else if (type == "uires")
        {
            float x = 0.0f, y = 0.0f;
            if (!TryReadExact(keystream, x, y) || x <= 0.0f || y <= 0.0f)
            {
                WarnWindowConfigLine(lineNumber, "Invalid UIRes (expected: UIRes <x> <y>, both > 0).");
                continue;
            }
            m_uiResolution = { x, y };
        }
        else if (type == "loglevel")
        {
            std::string levelStr;
            if (!TryReadExact(keystream, levelStr))
            {
                WarnWindowConfigLine(lineNumber, "Invalid LogLevel (expected: LogLevel Info|Warn|Error).");
                continue;
            }

            EngineLog::Level parsedLevel;
            if (!TryParseLogLevel(levelStr, parsedLevel))
            {
                WarnWindowConfigLine(lineNumber, "Unknown LogLevel '" + levelStr + "' (use Info/Warn/Error).");
                continue;
            }

            EngineLog::SetMinLevel(parsedLevel);
        }
        else if (type == "frameratelimit")
        {
            int limit = 0;
            if (!TryReadExact(keystream, limit) || limit < 0)
            {
                WarnWindowConfigLine(lineNumber, "Invalid FrameRateLimit (expected int >= 0, 0 = unlimited).");
                continue;
            }

            m_frameRateLimit = limit;
        }
        else if (type == "fullscreen")
        {
            int value = 0;
            if (!TryReadExact(keystream, value) || (value != 0 && value != 1))
            {
                WarnWindowConfigLine(lineNumber, "Invalid Fullscreen (expected 0 or 1).");
                continue;
            }

            m_isFullscreen = (value == 1);
        }
        else if (type == "resizable")
        {
            int value = 0;
            if (!TryReadExact(keystream, value) || (value != 0 && value != 1))
            {
                WarnWindowConfigLine(lineNumber, "Invalid Resizable (expected 0 or 1).");
                continue;
            }

            m_isResizeable = (value == 1);
        }
        else if (type == "windowres")
        {
            unsigned int x = 0, y = 0;
            if (!TryReadExact(keystream, x, y) || x == 0 || y == 0)
            {
                WarnWindowConfigLine(lineNumber, "Invalid WindowRes (expected: WindowRes <x> <y>, both > 0).");
                continue;
            }

            m_windowSize = { x, y };
        }
        else if (type == "aiseed")
        {
            std::string seedToken;
            if (!TryReadExact(keystream, seedToken))
            {
                WarnWindowConfigLine(lineNumber, "Invalid AISeed (expected: AISeed Auto|Random|<uint32>).");
                continue;
            }

            const std::string normalizedSeed = ToLowerCopy(seedToken);
            if (normalizedSeed == "auto" || normalizedSeed == "random")
            {
                m_hasFixedAISeed = false;
                m_fixedAISeed = 0;
            }
            else
            {
                try
                {
                    size_t parsedChars = 0;
                    const unsigned long long parsed = std::stoull(seedToken, &parsedChars);

                    if (parsedChars != seedToken.size() ||
                        parsed > std::numeric_limits<std::uint32_t>::max())
                    {
                        WarnWindowConfigLine(lineNumber, "AISeed out of range (expected uint32).");
                        continue;
                    }

                    m_fixedAISeed = static_cast<std::uint32_t>(parsed);
                    m_hasFixedAISeed = true;
                }
                catch (...)
                {
                    WarnWindowConfigLine(lineNumber, "Invalid AISeed '" + seedToken + "' (use Auto/Random or uint32).");
                    continue;
                }
            }
        }
        else
        {
            WarnWindowConfigLine(lineNumber, "Unknown key '" + rawType + "'.");
        }
    }
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
    // SFML 3 style vector calculations
    return sf::FloatRect({ viewCenter.x - viewSize.x / 2.0f, viewCenter.y - viewSize.y / 2.0f }, viewSize);
}