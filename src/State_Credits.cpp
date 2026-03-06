#include <iostream>
#include <cmath>
#include <cstdint>
#include "State_Credits.h"
#include "StateManager.h"
#include "SharedContext.h"
#include "Window.h"
#include "TextureManager.h"

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
    sf::Vector2f windowSize(m_stateManager.GetContext().m_window.GetWindowSize());
    m_view.setSize(windowSize);
    m_view.setCenter({ windowSize.x * 0.5f, windowSize.y * 0.5f });

    SharedContext& context = m_stateManager.GetContext();

    // Background and overlay
    context.m_textureManager.RequireResource("MenuBg");
    sf::Texture* bgTex = context.m_textureManager.GetResource("MenuBg");
    if (bgTex) {
        m_backgroundSprite.emplace(*bgTex);
        float scale = windowSize.y / bgTex->getSize().y;
        m_backgroundSprite->setScale({ scale, scale });
        m_backgroundSprite->setOrigin({ m_backgroundSprite->getLocalBounds().size.x * 0.5f, m_backgroundSprite->getLocalBounds().size.y * 0.5f });
        m_backgroundSprite->setPosition(m_view.getCenter());
    }

    m_overlay.setSize(windowSize);
    m_overlay.setFillColor(sf::Color(0, 0, 0, 180));

    // Load fonts
    if (!m_fontTitle.openFromFile("media/fonts/OLDENGL.ttf")) {
        std::cerr << "! Failed to load title font\n";
    }
    if (!m_fontBody.openFromFile("media/fonts/EightBitDragon.ttf")) {
        std::cerr << "! Failed to load body font\n";
    }

    // Text setup
    m_title.setString("CREDITS");
    m_title.setCharacterSize(80);
    sf::FloatRect titleRect = m_title.getLocalBounds();
    m_title.setOrigin({ titleRect.position.x + titleRect.size.x * 0.5f, titleRect.position.y + titleRect.size.y * 0.5f });
    m_title.setPosition({ m_view.getSize().x * 0.5f, m_view.getSize().y * 0.15f });

    m_creditsText.setString(CREDITS_CONTENT);
    m_creditsText.setCharacterSize(30);
    m_creditsText.setLineSpacing(1.5f);
    sf::FloatRect textRect = m_creditsText.getLocalBounds();
    m_creditsText.setOrigin({ textRect.position.x + textRect.size.x * 0.5f, 0.0f });

    m_scrollPos = m_view.getSize().y;
    m_creditsText.setPosition({ m_view.getSize().x * 0.5f, m_scrollPos });

    // Exit key
    m_promptText.setString("PRESS ESC TO RETURN");
    m_promptText.setCharacterSize(25);
    sf::FloatRect promptRect = m_promptText.getLocalBounds();
    m_promptText.setOrigin({ promptRect.position.x + promptRect.size.x * 0.5f, promptRect.position.y + promptRect.size.y * 0.5f });
    m_promptText.setPosition({ m_view.getSize().x * 0.5f, m_view.getSize().y * 0.9f });

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
        m_scrollPos = m_view.getSize().y;
    }

    // Blinking effect
    std::uint8_t alphaVal = static_cast<std::uint8_t>(255 * std::abs(std::sin(m_timePassed * 3.0f)));
    m_promptText.setFillColor(sf::Color(255, 255, 255, alphaVal));
}

void State_Credits::Draw()
{
    sf::RenderWindow& window = m_stateManager.GetContext().m_window.GetRenderWindow();
    window.setView(m_view);

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