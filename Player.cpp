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

void Player::Update(sf::Time deltaTime, const Map& map)
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

    // Move X
    m_position.x += m_velocity.x * dt;
    m_shape.setPosition(m_position); // Update position to check bounds
    ResolveCollisionsX(map);

    // Move Y
    m_position.y += m_velocity.y * dt;
    m_shape.setPosition(m_position);
    ResolveCollisionsY(map);

    // Sync visual shape with logical position
    m_shape.setPosition(m_position);
}

void Player::Draw(sf::RenderWindow& window)
{
    window.draw(m_shape);
}

void Player::ResolveCollisionsX(const Map& map)
{
    sf::FloatRect bounds = m_shape.getGlobalBounds();

    // Find the grid tiles the player is currently overlapping
    // In SFML 3, we use bounds.position and bounds.size instead of left/top/width/height
    // TODO: Ha senso?
    // We shrink the height slightly (-0.1f) so we don't get stuck on the floor when walking
    int leftTile = bounds.position.x / Map::TILE_SIZE;
    int rightTile = (bounds.position.x + bounds.size.x) / Map::TILE_SIZE;
    int topTile = bounds.position.y / Map::TILE_SIZE;
    int bottomTile = (bounds.position.y + bounds.size.y - 0.1f) / Map::TILE_SIZE;

    for (int y = topTile; y <= bottomTile; ++y)
    {
        for (int x = leftTile; x <= rightTile; ++x)
        {
            if (map.IsSolid(x, y))
            {
                if (m_velocity.x > 0.0f) // Moving right and hit a wall
                {
                    // Snap to the left of the wall (account for origin being at center X)
                    m_position.x = x * Map::TILE_SIZE - (bounds.size.x / 2.0f);
                    m_velocity.x = 0.0f;
                    return;
                }
                else if (m_velocity.x < 0.0f) // Moving left and hit a wall
                {
                    // Snap to the right of the wall
                    m_position.x = (x + 1) * Map::TILE_SIZE + (bounds.size.x / 2.0f);
                    m_velocity.x = 0.0f;
                    return;
                }
            }
        }
    }
}

void Player::ResolveCollisionsY(const Map& map)
{
    sf::FloatRect bounds = m_shape.getGlobalBounds();

    // Shrink the width slightly (+0.1f / -0.1f) to avoid touching walls when falling into a tight hole.
    int leftTile = (bounds.position.x + 0.1f) / Map::TILE_SIZE;
    int rightTile = (bounds.position.x + bounds.size.x - 0.1f) / Map::TILE_SIZE;
    int topTile = bounds.position.y / Map::TILE_SIZE;
    int bottomTile = (bounds.position.y + bounds.size.y) / Map::TILE_SIZE;

    // Assume we are in the air at the start of the check
    m_isGrounded = false;

    for (int y = topTile; y <= bottomTile; ++y)
    {
        for (int x = leftTile; x <= rightTile; ++x)
        {
            if (map.IsSolid(x, y))
            {
                if (m_velocity.y > 0.0f) // Falling down and hit the floor
                {
                    // Snap to the top of the floor tile (origin is at bottom Y)
                    m_position.y = y * Map::TILE_SIZE;
                    m_velocity.y = 0.0f;
                    m_isGrounded = true;
                    m_coyoteTimer = COYOTE_TIME; // Recharge coyote time
                    return;
                }
                else if (m_velocity.y < 0.0f) // Jumping up and hit the ceiling (bonk!)
                {
                    // Snap to the bottom of the ceiling tile
                    m_position.y = (y + 1) * Map::TILE_SIZE + bounds.size.y;
                    m_velocity.y = 0.0f; // Reset vertical velocity to fall immediately
                    return;
                }
            }
        }
    }
}