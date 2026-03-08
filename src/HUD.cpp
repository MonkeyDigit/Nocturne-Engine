#include <algorithm>
#include "HUD.h"
#include "EntityManager.h"
#include "EntityBase.h"
#include "CState.h"
#include "Utilities.h"

namespace
{
    constexpr float kHealthBarWidth = 200.0f;
    constexpr float kHealthBarHeight = 20.0f;
    constexpr float kHealthBarX = 20.0f;
    constexpr float kHealthBarY = 20.0f;

    constexpr float kHealthLabelX = 20.0f;
    constexpr float kHealthLabelY = 0.0f;
    constexpr unsigned int kHealthLabelCharacterSize = 18u;
    constexpr float kHealthLabelOutlineThickness = 1.0f;
}

HUD::HUD(EntityManager& entityManager)
    : m_entityManager(entityManager), m_healthLabel(m_font), m_fontLoaded(false)
{
    // Initialize static HUD geometry once
    m_healthBarBackground.setSize({ kHealthBarWidth, kHealthBarHeight });
    m_healthBarBackground.setPosition({ kHealthBarX, kHealthBarY });
    m_healthBarBackground.setFillColor(sf::Color(35, 35, 35, 220));
    m_healthBarBackground.setOutlineColor(sf::Color::Black);
    m_healthBarBackground.setOutlineThickness(1.0f);

    m_healthBar.setSize({ kHealthBarWidth, kHealthBarHeight });
    m_healthBar.setPosition({ kHealthBarX, kHealthBarY });
    m_healthBar.setFillColor(sf::Color(200, 45, 45, 240));

    m_fontLoaded = Utils::LoadFontOrWarn(
        m_font,
        "media/fonts/EightBitDragon.ttf",
        "HUD",
        "main");

    if (m_fontLoaded)
    {
        m_healthLabel.setString("Health");
        m_healthLabel.setCharacterSize(kHealthLabelCharacterSize);
        m_healthLabel.setFillColor(sf::Color::White);
        m_healthLabel.setOutlineColor(sf::Color::Black);
        m_healthLabel.setOutlineThickness(kHealthLabelOutlineThickness);
        m_healthLabel.setPosition({ kHealthLabelX, kHealthLabelY });
    }
}

void HUD::Update()
{
    EntityBase* player = m_entityManager.GetPlayer();
    if (player)
    {
        CState* state = player->GetComponent<CState>();
        if (state)
        {
            const int maxHealth = state->GetMaxHitPoints();
            const int currentHealth = std::clamp(state->GetHitPoints(), 0, maxHealth);
            const float healthRatio = (maxHealth > 0)
                ? static_cast<float>(currentHealth) / static_cast<float>(maxHealth)
                : 0.0f;

            m_healthBar.setSize({ kHealthBarWidth * healthRatio, kHealthBarHeight });
            return;
        }
    }

    // No valid player/state: render empty bar
    m_healthBar.setSize({ 0.0f, kHealthBarHeight });
}

void HUD::Draw(sf::RenderWindow& window)
{
    if (m_fontLoaded)
        window.draw(m_healthLabel);

    window.draw(m_healthBarBackground);
    window.draw(m_healthBar);
}