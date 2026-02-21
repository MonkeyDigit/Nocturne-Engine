#include "Player.h"
#include <cmath>
#include <algorithm>

Player::Player()
    : m_position(640.0f, 360.0f), // Start in the middle of the screen
    m_velocity(0.0f, 0.0f)
{
    // A standard humanoid hitbox size
    m_shape.setSize({ 32.0f, 64.0f });
    // Set origin to bottom-center for easier floor alignment later
    m_shape.setOrigin({ 16.0f, 64.0f });
    m_shape.setFillColor(sf::Color::White);
    m_shape.setPosition(m_position);
}

void Player::Update(sf::Time deltaTime)
{
    float dt = deltaTime.asSeconds();
    float moveDirection = 0.0f;

    // 1. Get Input
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
    {
        moveDirection -= 1.0f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
    {
        moveDirection += 1.0f;
    }

    // 2. Apply Acceleration or Friction
    if (moveDirection != 0.0f)
    {
        // Apply acceleration
        m_velocity.x += moveDirection * ACCELERATION * dt;
    }
    else
    {
        // Apply friction when no input is pressed
        if (m_velocity.x > 0.0f)
        {
            m_velocity.x -= FRICTION * dt;
            if (m_velocity.x < 0.0f) m_velocity.x = 0.0f; // Stop exactly at 0
        }
        else if (m_velocity.x < 0.0f)
        {
            m_velocity.x += FRICTION * dt;
            if (m_velocity.x > 0.0f) m_velocity.x = 0.0f; // Stop exactly at 0
        }
    }

    // 3. Clamp Velocity to MaxSpeed
    m_velocity.x = std::clamp(m_velocity.x, -MAX_SPEED, MAX_SPEED);

    // 4. Update Position
    m_position.x += m_velocity.x * dt;

    // Sync visual shape with logical position
    m_shape.setPosition(m_position);
}

void Player::Draw(sf::RenderWindow& window)
{
    window.draw(m_shape);
}