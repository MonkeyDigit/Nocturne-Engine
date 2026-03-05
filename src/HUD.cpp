#include "HUD.h"
#include "EntityManager.h"
#include "EntityBase.h"
#include "CState.h"

HUD::HUD(EntityManager& entityManager)
    : m_entityManager(entityManager), m_maxHealth(0)
{
    // Setup the Background Bar (Red - shows empty health)
    m_healthBarBackground.setSize({ 200.0f, 20.0f }); // SFML 3 uses {}
    m_healthBarBackground.setFillColor(sf::Color::Red);
    m_healthBarBackground.setPosition({ 20.0f, 20.0f });

    // Setup the Foreground Bar (Green - shows current health)
    m_healthBar.setSize({ 200.0f, 20.0f });
    m_healthBar.setFillColor(sf::Color::Green);
    m_healthBar.setPosition({ 20.0f, 20.0f });
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
    // Save the current camera view (the one following the player)
    sf::View cameraView = window.getView();

    // Switch to the default view (fixed to the window's top-left corner)
    window.setView(window.getDefaultView());

    // Draw the UI
    window.draw(m_healthBarBackground);
    window.draw(m_healthBar);

    // Restore the camera view for the rest of the game
    window.setView(cameraView);
}