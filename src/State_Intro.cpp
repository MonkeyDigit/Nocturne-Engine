#include <iostream>
#include <cmath>
#include <cstdint>
#include "State_Intro.h"
#include "StateManager.h"
#include "SharedContext.h"
#include "Window.h"
#include "TextureManager.h"

State_Intro::State_Intro(StateManager& stateManager)
    : BaseState(stateManager), m_text(m_font), m_timePassed(0.0f), m_alpha(90.0f)
{
}

State_Intro::~State_Intro() {}

void State_Intro::OnCreate()
{
    sf::Vector2f windowSize(m_stateManager.GetContext().m_window.GetWindowSize());
    m_view.setSize(windowSize);
    m_view.setCenter({ windowSize.x * 0.5f, windowSize.y * 0.5f });

    SharedContext& context = m_stateManager.GetContext();
    TextureManager& textureMgr = context.m_textureManager;

    textureMgr.RequireResource("Intro");
    textureMgr.RequireResource("MenuBg");

    sf::Texture* bgTex = textureMgr.GetResource("MenuBg");
    if (bgTex) {
        m_backgroundSprite.emplace(*bgTex);
        float scale = windowSize.y / bgTex->getSize().y;
        m_backgroundSprite->setScale({ scale, scale });
        m_backgroundSprite->setOrigin({ m_backgroundSprite->getLocalBounds().size.x * 0.5f, m_backgroundSprite->getLocalBounds().size.y * 0.5f });
        m_backgroundSprite->setPosition(m_view.getCenter());
    }

    sf::Texture* introText = textureMgr.GetResource("Intro");
    if (introText)
    {
        m_introSprite.emplace(*introText);
        m_introSprite->setOrigin({ introText->getSize().x * 0.5f, introText->getSize().y * 0.5f });
        m_introSprite->setPosition({ m_view.getCenter().x, -(int)introText->getSize().y * 0.5f });
    }

    if (!m_font.openFromFile("media/fonts/EightBitDragon.ttf")) {
        std::cerr << "! Failed to load font for Intro\n";
    }

    m_text.setString("PRESS SPACE TO CONTINUE");
    m_text.setCharacterSize(30);
    sf::FloatRect textRect = m_text.getLocalBounds();
    m_text.setOrigin({ textRect.position.x + textRect.size.x * 0.5f,
                       textRect.position.y + textRect.size.y * 0.5f });
    m_text.setPosition({ m_view.getCenter().x, m_view.getSize().y * 0.8f });

    context.m_eventManager.AddCallback(StateType::Intro, "Intro_Continue", &State_Intro::Continue, *this);
}

void State_Intro::OnDestroy()
{
    SharedContext& context = m_stateManager.GetContext();
    context.m_eventManager.RemoveCallback(StateType::Intro, "Intro_Continue");
    context.m_textureManager.ReleaseResource("Intro");
    context.m_textureManager.ReleaseResource("MenuBg");
}

void State_Intro::Activate() {}
void State_Intro::Deactivate() {}

void State_Intro::Update(const sf::Time& time)
{
    float dt = time.asSeconds();
    m_timePassed += dt;

    if (m_introSprite) {
        if (m_timePassed < ANIMATION_DURATION)
        {

            float targetY = m_view.getSize().y * 0.5f;
            float startY = -(int)m_introSprite->getTexture().getSize().y * 0.5f;
            float distance = targetY - startY;
            float speed = distance / ANIMATION_DURATION;

            m_introSprite->move({ 0.0f, speed * dt });
        }
        else
        {
            m_introSprite->setPosition({ m_introSprite->getPosition().x, m_view.getSize().y * 0.5f });

            m_alpha += PULSE_SPEED * dt;
            std::uint8_t alphaVal = static_cast<std::uint8_t>(255 * std::abs(std::sin(m_alpha * 3.14159f / 180.0f)));
            m_text.setFillColor(sf::Color(255, 255, 255, alphaVal));
        }
    }
}

void State_Intro::Draw()
{
    sf::RenderWindow& window = m_stateManager.GetContext().m_window.GetRenderWindow();
    window.setView(m_view);

    if (m_backgroundSprite) window.draw(*m_backgroundSprite);
    if (m_introSprite) window.draw(*m_introSprite);
    if (m_timePassed >= ANIMATION_DURATION) window.draw(m_text);
}

void State_Intro::Continue(EventDetails& details)
{
    m_stateManager.SwitchTo(StateType::MainMenu);
    m_stateManager.Remove(StateType::Intro);
}