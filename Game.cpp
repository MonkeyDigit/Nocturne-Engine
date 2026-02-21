#include "Game.h"

Game::Game()
// SFML 3 uses braced initialization for sf::VideoMode
    : m_window(sf::VideoMode({ 1280, 720 }), "Project Nocturne"),
    m_enemy(800.0f, 300.0f),
    m_boss(1000.0f, 300.0f),
    m_bossDefeatedProcessed(false)
{
    // Disable VSync to test the robustness of our fixed timestep loop
    m_window.setVerticalSyncEnabled(false);
}

void Game::Run()
{
    sf::Clock clock;
    sf::Time timeSinceLastUpdate = sf::Time::Zero;

    while (m_window.isOpen())
    {
        // Measure the time passed since the last loop iteration
        sf::Time elapsedTime = clock.restart();
        timeSinceLastUpdate += elapsedTime;

        // ACCUMULATOR PATTERN:
        // Consume the accumulated time in discrete 1/60s chunks
        // This ensures the physics run at a constant rate regardless of frame rate
        while (timeSinceLastUpdate > TIME_PER_FRAME)
        {
            timeSinceLastUpdate -= TIME_PER_FRAME;

            ProcessEvents();
            // We ALWAYS pass the fixed constant to the Update method
            Update(TIME_PER_FRAME);
        }

        // Render as fast as possible
        Render();
    }
}

void Game::ProcessEvents()
{
    // SFML 3 uses std::optional for event polling
    while (const std::optional event = m_window.pollEvent())
    {
        // Type-safe event checking using templates
        if (event->is<sf::Event::Closed>())
        {
            m_window.close();
        }
        // TODO: Future input bindings (Key pressed/released) will go here
    }
}

void Game::Update(sf::Time deltaTime)
{
    // TODO: Here we will update our mathematical Player logic later
    m_player.Update(deltaTime, m_map);
    m_enemy.Update(deltaTime, m_map);
    // Pass the player's position to the boss
    m_boss.Update(deltaTime, m_map, m_player.GetPosition());

    // --- COMBAT RESOLUTION ---
    // If the player is currently throwing out an attack hitbox
    if (m_player.IsAttacking())
    {
        sf::FloatRect attackBounds = m_player.GetAttackBounds();
        sf::FloatRect enemyBounds = m_enemy.GetBounds();

        // Check Enemy Collision
        // In SFML 3, findIntersection returns std::optional<sf::FloatRect>.
        // If it has a value, they are colliding
        if (attackBounds.findIntersection(enemyBounds).has_value())
        {
            // Push the enemy in the exact direction the player is facing
            float knockbackDir = static_cast<float>(m_player.GetFacingDirection());

            // Deal 1 damage and apply knockback
            m_enemy.TakeDamage(1, knockbackDir);
        }

        // Check Boss Collision
        if (!m_boss.IsDead() && attackBounds.findIntersection(m_boss.GetBounds()).has_value())
        {
            float knockbackDir = static_cast<float>(m_player.GetFacingDirection());
            m_boss.TakeDamage(1, knockbackDir);
        }
    }

    // --- GATING LOGIC (Unlock Double Jump) ---
    if (m_boss.IsDead() && !m_bossDefeatedProcessed)
    {
        m_player.UnlockDoubleJump();
        m_bossDefeatedProcessed = true;
    }
}

void Game::Render()
{
    m_window.clear(sf::Color(30, 30, 30));
    // TODO: Drawing of map and entities will happen here

    m_map.Draw(m_window);       // First draw the map
    m_boss.Draw(m_window);
    m_enemy.Draw(m_window); // Draw enemy behind the player
    m_player.Draw(m_window);    // Then the player

    m_window.display();
}