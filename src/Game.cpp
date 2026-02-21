#include "Game.h"

Game::Game()
// SFML 3 uses braced initialization for sf::VideoMode
    : m_window("Project Nocturne", { 1280, 720 }),
    m_enemy(400.0f, 150.0f),
    m_boss(120.0f, 400.0f),
    m_bossDefeatedProcessed(false)
{
    // Setup camera with 2x zoom
    m_camera.setSize({ 640.0f, 360.0f });
}

void Game::Run()
{
    sf::Clock clock;
    sf::Time timeSinceLastUpdate = sf::Time::Zero;

    while (!m_window.IsDone())
    {
        // Measure the time passed since the last loop iteration
        sf::Time elapsedTime = clock.restart();
        timeSinceLastUpdate += elapsedTime;

        // ACCUMULATOR PATTERN:
        // Consume the accumulated time in discrete 1/60s chunks
        // This ensures the physics run at a constant rate regardless of frame rate
        if(timeSinceLastUpdate > TIME_PER_FRAME)
        {
            timeSinceLastUpdate -= TIME_PER_FRAME;

            // We ALWAYS pass the fixed constant to the Update method
            Update(TIME_PER_FRAME);
        }

        // Render as fast as possible
        Render();
    }
}

void Game::Update(sf::Time deltaTime)
{
    // First update the window
    m_window.Update();
    // Update Input states
    m_eventManager.Update();

    // TODO: Da migliorare
    // Pass EventManager to Player so it can read inputs
    m_player.Update(deltaTime, m_map, m_eventManager);
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

    // --- CAMERA LOGIC ---
    sf::Vector2f playerPos = m_player.GetPosition();

    // We center the camera on the player
    sf::Vector2f cameraCenter = playerPos;

    // Clamp camera to map bounds so it doesn't show areas outside the map
    float halfWidth = m_camera.getSize().x / 2.0f;
    float halfHeight = m_camera.getSize().y / 2.0f;

    if (cameraCenter.x < halfWidth) cameraCenter.x = halfWidth;
    if (cameraCenter.x > m_map.GetWidth() - halfWidth) cameraCenter.x = m_map.GetWidth() - halfWidth;

    if (cameraCenter.y < halfHeight) cameraCenter.y = halfHeight;
    if (cameraCenter.y > m_map.GetHeight() - halfHeight) cameraCenter.y = m_map.GetHeight() - halfHeight;

    m_camera.setCenter(cameraCenter);
}

void Game::Render()
{
    m_window.BeginDraw();
    // Apply Camera View before rendering
    m_window.GetRenderWindow().setView(m_camera);
    // TODO: Drawing of map and entities will happen here

    m_map.Draw(m_window.GetRenderWindow());         // First draw the map
    m_boss.Draw(m_window.GetRenderWindow());
    m_enemy.Draw(m_window.GetRenderWindow());       // Draw enemy behind the player
    m_player.Draw(m_window.GetRenderWindow());      // Then the player

    m_window.EndDraw();
}