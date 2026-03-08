#include <cmath>
#include <cstdint>
#include "State_Credits.h"
#include "StateManager.h"
#include "SharedContext.h"
#include "Window.h"

State_Credits::State_Credits(StateManager& stateManager)
    : BaseState(stateManager),
    m_title(m_fontTitle),
    m_creditsText(m_fontBody),
    m_promptText(m_fontBody),
    m_scrollPos(0.0f),
    m_timePassed(0.0f)
{
}

State_Credits::~State_Credits() {}

void State_Credits::OnCreate()
{
    SharedContext& context = m_stateManager.GetContext();
    sf::Vector2f uiRes = context.m_window.GetUIResolution();

    // Background and overlay
    SetupCenteredBackground(
        m_backgroundSprite,
        "MenuBg",
        uiRes,
        "State_Credits");

    m_overlay.setSize(uiRes);
    m_overlay.setFillColor(sf::Color(0, 0, 0, 180));

    // Load fonts
    LoadFontOrWarn(m_fontTitle, "media/fonts/EightBitDragon.ttf", "State_Credits", "title");
    LoadFontOrWarn(m_fontBody, "media/fonts/EightBitDragon.ttf", "State_Credits", "body");

    // Text setup
    m_title.setString("CREDITS");
    m_title.setCharacterSize(80);
    CenterText(m_title, uiRes.x * 0.5f, uiRes.y * 0.2f);

    m_creditsText.setString(CREDITS_CONTENT);
    m_creditsText.setCharacterSize(30);
    m_creditsText.setLineSpacing(1.5f);
    sf::FloatRect textRect = m_creditsText.getLocalBounds();
    m_creditsText.setOrigin({ textRect.position.x + textRect.size.x * 0.5f, 0.0f });

    m_scrollPos = uiRes.y;
    m_creditsText.setPosition({ uiRes.x * 0.5f, m_scrollPos });

    // Exit key
    m_promptText.setString("PRESS ESC TO RETURN");
    m_promptText.setCharacterSize(25);
    CenterText(m_promptText, uiRes.x * 0.5f, uiRes.y * 0.9f);

    // Exit callback
    context.m_eventManager.AddCallback(StateType::Credits, "Credits_Back", &State_Credits::ReturnToMenu, *this);
}

void State_Credits::OnDestroy()
{
    SharedContext& context = m_stateManager.GetContext();
    context.m_eventManager.RemoveCallback(StateType::Credits, "Credits_Back");
    ReleaseTrackedTextures("State_Credits");
}

void State_Credits::Activate() {}
void State_Credits::Deactivate() {}

void State_Credits::Update(const sf::Time& time)
{
    float dt = time.asSeconds();
    m_timePassed += dt;

    // Scrolling logic
    m_scrollPos -= SCROLL_SPEED * dt;
    m_creditsText.setPosition({ m_creditsText.getPosition().x, m_scrollPos });

    // Animation loop
    if (m_scrollPos < -m_creditsText.getLocalBounds().size.y) {
        m_scrollPos = m_stateManager.GetContext().m_window.GetUIResolution().y;
    }

    // Blinking effect
    std::uint8_t alphaVal = static_cast<std::uint8_t>(255 * std::abs(std::sin(m_timePassed * 3.0f)));
    m_promptText.setFillColor(sf::Color(255, 255, 255, alphaVal));
}

void State_Credits::Draw()
{
    sf::RenderWindow& window = m_stateManager.GetContext().m_window.GetRenderWindow();
    window.setView(m_stateManager.GetContext().m_window.GetUIView());

    if (m_backgroundSprite) window.draw(*m_backgroundSprite);
    window.draw(m_overlay);
    window.draw(m_title);
    window.draw(m_creditsText);
    window.draw(m_promptText);
}

void State_Credits::ReturnToMenu(EventDetails& details)
{
    m_stateManager.SwitchTo(StateType::MainMenu);
}