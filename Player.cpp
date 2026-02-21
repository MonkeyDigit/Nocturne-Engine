#include "Player.h"
#include <cmath>
#include <algorithm>

Player::Player()
    : m_position(640.0f, 360.0f), // Start in the middle of the screen
    m_velocity(0.0f, 0.0f),
    m_isGrounded(false),
    m_wasJumpPressed(false),
    m_coyoteTimer(0.0f),
    m_jumpBufferTimer(0.0f)
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

    // --- TIMERS UPDATE ---
    m_coyoteTimer -= dt;
    m_jumpBufferTimer -= dt;

    // Get Input
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
    {
        moveDirection -= 1.0f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
    {
        moveDirection += 1.0f;
    }

    // Apply Acceleration or Friction
    if (moveDirection != 0.0f)
    {
        // TODO: In Phase 2, we will apply AirControlMultiplier if !m_isGrounded
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

    // Clamp Velocity to MaxSpeed
    m_velocity.x = std::clamp(m_velocity.x, -MAX_SPEED, MAX_SPEED);

    // --- JUMP INPUT LOGIC ---
    // Let's use 'Z' or 'Space' for jumping
    bool jumpKeyPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Z) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);

    // Jump Buffer: Set timer when key is JUST pressed
    if (jumpKeyPressed && !m_wasJumpPressed)
    {
        m_jumpBufferTimer = JUMP_BUFFER_TIME;
    }

    // Variable Jump Height: Cut velocity if key is JUST released while going up
    if (!jumpKeyPressed && m_wasJumpPressed && m_velocity.y < 0.0f)
    {
        m_velocity.y *= JUMP_CUT_MULTIPLIER;
    }

    m_wasJumpPressed = jumpKeyPressed; // Store for next frame

    // --- GRAVITY ---
    m_velocity.y += GRAVITY * dt;

    // --- EXECUTE JUMP ---
    // If the player wants to jump (buffer) AND is allowed to jump (coyote)
    if (m_jumpBufferTimer > 0.0f && m_coyoteTimer > 0.0f)
    {
        m_velocity.y = -JUMP_FORCE; // Negative Y is up in SFML

        // Reset timers so we don't double jump instantly
        m_jumpBufferTimer = 0.0f;
        m_coyoteTimer = 0.0f;
    }

    // --- APPLY VELOCITY ---
    m_position.x += m_velocity.x * dt;
    m_position.y += m_velocity.y * dt;

    // --- MOCK FLOOR COLLISION (For Phase 1 Testing) ---
    const float FLOOR_Y = 600.0f; // Arbitrary ground level

    if (m_position.y >= FLOOR_Y)
    {
        m_position.y = FLOOR_Y;
        m_velocity.y = 0.0f;
        m_isGrounded = true;

        // As long as we are on the ground, Coyote Time is full
        m_coyoteTimer = COYOTE_TIME;
    }
    else
    {
        m_isGrounded = false;
    }

    // Sync visual shape with logical position
    m_shape.setPosition(m_position);
}

void Player::Draw(sf::RenderWindow& window)
{
    window.draw(m_shape);
}