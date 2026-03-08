#include <cmath>
#include <cstdint>
#include "State_Credits.h"
#include "StateManager.h"
#include "SharedContext.h"
#include "Window.h"
#include "TextureManager.h"
#include "EngineLog.h"

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
    context.m_textureManager.RequireResource("MenuBg");
    sf::Texture* bgTex = context.m_textureManager.GetResource("MenuBg");
    if (bgTex)
    {
        m_backgroundSprite.emplace(*bgTex);
        m_backgroundSprite->setOrigin({ m_backgroundSprite->getLocalBounds().size.x * 0.5f, m_backgroundSprite->getLocalBounds().size.y * 0.5f });
        m_backgroundSprite->setPosition(uiRes * 0.5f);
    }

    m_overlay.setSize(uiRes);
    m_overlay.setFillColor(sf::Color(0, 0, 0, 180));

    // Load fonts
    if (!m_fontTitle.openFromFile("media/fonts/EightBitDragon.ttf"))
        EngineLog::WarnOnce("font.credits.title_failed", "Failed to load title font");
    if (!m_fontBody.openFromFile("media/fonts/EightBitDragon.ttf"))
        EngineLog::WarnOnce("font.credits.body_failed", "Failed to load body font");

    // Text setup
    m_title.setString("CREDITS");
    m_title.setCharacterSize(80);
    sf::FloatRect titleRect = m_title.getLocalBounds();
    m_title.setOrigin({ titleRect.position.x + titleRect.size.x * 0.5f, titleRect.position.y + titleRect.size.y * 0.5f });
    m_title.setPosition({ uiRes.x * 0.5f, uiRes.y * 0.2f });

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
    sf::FloatRect promptRect = m_promptText.getLocalBounds();
    m_promptText.setOrigin({ promptRect.position.x + promptRect.size.x * 0.5f, promptRect.position.y + promptRect.size.y * 0.5f });
    m_promptText.setPosition({ uiRes.x * 0.5f, uiRes.y * 0.9f });

    // Exit callback
    context.m_eventManager.AddCallback(StateType::Credits, "Credits_Back", &State_Credits::ReturnToMenu, *this);
}

void State_Credits::OnDestroy()
{
    SharedContext& context = m_stateManager.GetContext();
    context.m_eventManager.RemoveCallback(StateType::Credits, "Credits_Back");
    context.m_textureManager.ReleaseResource("MenuBg");
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