#include <iostream>
#include "HUD.h"
#include "EntityManager.h"
#include "EntityBase.h"
#include "CState.h"

HUD::HUD(EntityManager& entityManager)
    : m_entityManager(entityManager), m_maxHealth(0), m_healthLabel(m_font), m_fontLoaded(false)
{
    m_healthBarBackground.setSize({ 200.0f, 20.0f });
    m_healthBarBackground.setFillColor(sf::Color::Red);
    m_healthBarBackground.setPosition({ 20.0f, 20.0f });

    m_healthBar.setSize({ 200.0f, 20.0f });
    m_healthBar.setFillColor(sf::Color::Green);
    m_healthBar.setPosition({ 20.0f, 20.0f });

    m_fontLoaded = m_font.openFromFile("media/fonts/EightBitDragon.ttf");
    if (m_fontLoaded)
    {
        m_healthLabel.setString("Health");
        m_healthLabel.setCharacterSize(18);
        m_healthLabel.setFillColor(sf::Color::White);
        m_healthLabel.setOutlineColor(sf::Color::Black);
        m_healthLabel.setOutlineThickness(1.0f);
        m_healthLabel.setPosition({ 20.0f, 0.0f });
    }
    else
        std::cerr << "! Failed to load HUD font\n";
}

void HUD::Update()
{
    EntityBase* player = m_entityManager.Find("Player");
    if (player)
    {
        CState* state = player->GetComponent<CState>();
        if (state)
        {
            // Capture the max health the first time we update
            if (m_maxHealth == 0) m_maxHealth = state->GetHitPoints();

            // Calculate the percentage of health remaining
            float healthRatio = 0.0f;
            if (m_maxHealth > 0)
            {
                healthRatio = static_cast<float>(state->GetHitPoints()) / m_maxHealth;
                if (healthRatio < 0.0f) healthRatio = 0.0f; // Prevent negative width
            }

            // Shrink the green bar accordingly
            m_healthBar.setSize({ 200.0f * healthRatio, 20.0f });
        }
    }
    else
    {
        // If player is dead, empty the bar
        m_healthBar.setSize({ 0.0f, 20.0f });
        m_maxHealth = 0; // Reset so it recalculates on respawn
    }
}

void HUD::Draw(sf::RenderWindow& window)
{
    if (m_fontLoaded)
        window.draw(m_healthLabel);

    window.draw(m_healthBarBackground);
    window.draw(m_healthBar);
}