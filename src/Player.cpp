#include "Player.h"
#include <cmath>
#include <iostream>
#include <algorithm>

Player::Player()
    : m_sprite(m_texture), 
    m_position(64.0f, 64.0f),
    m_velocity(0.0f, 0.0f),
    m_isGrounded(false),
    m_coyoteTimer(0.0f),
    m_jumpBufferTimer(0.0f),
    m_facingDirection(1), // Start facing right
    m_isAttacking(false),
    m_attackTimer(0.0f),
    m_hasDoubleJumpUnlocked(false), // TODO: Unlocked by killing the boss
    m_canDoubleJump(false),
    m_animState(AnimState::Idle),
    m_animTimer(0.0f),
    m_currentFrame(0)
{
    // A standard humanoid hitbox size
    m_shape.setSize({ 16.0f, 42.0f });
    // Set origin to bottom-center for easier floor alignment later
    m_shape.setOrigin({ 8.0f, 42.0f });
    // --- DEBUG HITBOX ---
    // Make the physical shape semi-transparent red with an outline
    // so we can see the sprite AND the collision box simultaneously
    m_shape.setFillColor(sf::Color(255, 0, 0, 80));
    //Use negative thickness for an internal outline!
    // A positive outline expands getGlobalBounds(), making the player collide with the floor during X checks!
    m_shape.setOutlineThickness(-1.0f);
    m_shape.setOutlineColor(sf::Color::Red);

    // Setup Attack Hitbox (e.g. a sword thrust)
    m_attackHitbox.setSize({ 24.0f, 8.0f });
    m_attackHitbox.setOrigin({ 0.0f, 4.0f });
    m_attackHitbox.setFillColor(sf::Color(255, 200, 0, 150)); // Semi-transparent yellow

    // --- LOAD TEXTURE ---
    if (!m_texture.loadFromFile("media/textures/PlayerSheet.png"))
    {
        std::cerr << "ERROR: Could not load PlayerSheet.png!\n";
    }

    // Origin is bottom-center, just like the physics hitbox, so they align automatically
    m_sprite.setOrigin({ SPRITE_WIDTH / 2.0f, (float)SPRITE_HEIGHT });
    m_sprite.setTextureRect(sf::IntRect({ 0, 0 }, { SPRITE_WIDTH, SPRITE_HEIGHT }));
}

void Player::Update(sf::Time deltaTime, const Map& map, const EventManager& input)
{
    float dt = deltaTime.asSeconds();
    float moveDirection = 0.0f;

    // --- TIMERS UPDATE ---
    m_coyoteTimer -= dt;
    m_jumpBufferTimer -= dt;

    // TODO: TOGLI LE STRINGHE
    // --- HORIZONTAL INPUT ---
    if (input.IsActionHeld("MoveLeft"))
    {
        moveDirection -= 1.0f;
        m_facingDirection = -1;
    }
    if (input.IsActionHeld("MoveRight"))
    {
        moveDirection += 1.0f;
        m_facingDirection = 1;
    }

    // Apply Acceleration or Friction
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

    // Clamp Velocity to MaxSpeed
    m_velocity.x = std::clamp(m_velocity.x, -MAX_SPEED, MAX_SPEED);

    // --- JUMP INPUT LOGIC ---
    // Jump Buffer: Set timer when key is JUST pressed
    if (input.IsActionJustPressed("Jump"))
    {
        m_jumpBufferTimer = JUMP_BUFFER_TIME;
    }

    // Variable Jump Height: Cut velocity if key is JUST released while going up
    if (input.IsActionJustReleased("Jump") && m_velocity.y < 0.0f)
    {
        m_velocity.y *= JUMP_CUT_MULTIPLIER;
    }

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
    // Double Jump (In the air)
    else if (m_jumpBufferTimer > 0.0f && m_coyoteTimer <= 0.0f)
    {
        if (m_hasDoubleJumpUnlocked && m_canDoubleJump)
        {
            // Reset vertical velocity completely, then apply jump force
            // This ensures consistent height regardless of falling speed
            m_velocity.y = -DOUBLE_JUMP_FORCE;

            m_jumpBufferTimer = 0.0f;
            m_canDoubleJump = false; // Consume the double jump
        }
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

    // --- COMBAT LOGIC ---

    // Start attack (using 'X' key)
    bool attackKeyPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::X);

    if (m_isAttacking)
    {
        m_attackTimer -= dt;
        if (m_attackTimer <= 0.0f)
        {
            m_isAttacking = false;
        }
    }
    else
    {
        if (input.IsActionJustPressed("Attack"))
        {
            m_isAttacking = true;
            m_attackTimer = ATTACK_DURATION;
        }
    }

    // Update Attack Hitbox position based on player position and facing direction
    if (m_facingDirection == 1)
    {
        // Spawns on the right side
        m_attackHitbox.setPosition({ m_position.x + 8.0f, m_position.y - 16.0f });
        // We flip the size to point right
        m_attackHitbox.setScale({ 1.0f, 1.0f });
    }
    else
    {
        // Spawns on the left side
        m_attackHitbox.setPosition({ m_position.x - 8.0f, m_position.y - 16.0f });
        // We flip the scale to point left (SFML 3 handles negative scale well)
        m_attackHitbox.setScale({ -1.0f, 1.0f });
    }

    // ==========================================
    // VISUAL ANIMATION LOGIC
    // ==========================================

    // Determine the current logical state
    AnimState nextState = AnimState::Idle;

    if (m_isAttacking)
        nextState = AnimState::Attack;
    else if (!m_isGrounded)
    {
        if (m_velocity.y < 0.0f) nextState = AnimState::Jump;
        else nextState = AnimState::Fall;
    }
    else if (std::abs(m_velocity.x) > 10.0f)
        nextState = AnimState::Run;

    // Mapping the states
    int row = 0;
    int startFrame = 0;
    int endFrame = 0;
    float frameTime = 0.06f;
    bool loop = true;

    if (nextState == AnimState::Idle) {
        row = 0; startFrame = 0; endFrame = 5; frameTime = 0.16f; loop = true;
    }
    else if (nextState == AnimState::Run) {
        row = 2; startFrame = 0; endFrame = 14; frameTime = 0.04f; loop = true;
    }
    else if (nextState == AnimState::Jump) {
        row = 12; startFrame = 0; endFrame = 5; frameTime = 0.06f; loop = false;
    }
    else if (nextState == AnimState::Fall) {
        row = 12; startFrame = 7; endFrame = 15; frameTime = 0.06f; loop = false;
    }
    else if (nextState == AnimState::Attack) {
        row = 6; startFrame = 13; endFrame = 17; frameTime = 0.06f; loop = false;
    }

    // Reset frame if state changed
    if (nextState != m_animState)
    {
        m_animState = nextState;
        m_currentFrame = 0;
        m_animTimer = 0.0f;
    }

    // Update animation timer
    m_animTimer += dt;
    if (m_animTimer > 0.1f) // Change frame every 0.1 seconds (10 fps)
    {
        m_animTimer = 0.0f;
        m_currentFrame++;

        // Loop back
        if (m_currentFrame > endFrame)
        {
            if (loop) {
                m_currentFrame = startFrame;
            }
            else {
                m_currentFrame = endFrame;
            }
        }
    }

    // Frame crop
    m_sprite.setTextureRect(sf::IntRect(
        { m_currentFrame * SPRITE_WIDTH, row * SPRITE_HEIGHT },
        { SPRITE_WIDTH, SPRITE_HEIGHT }
    ));

    // Synchronize the frame to logical position
    m_sprite.setPosition(m_position);

    // Turn the frame left or right
    if (m_facingDirection == 1)
        m_sprite.setScale({ 1.0f, 1.0f });
    else
        m_sprite.setScale({ -1.0f, 1.0f });
}

void Player::Draw(sf::RenderWindow& window)
{
    // Draw the sprite FIRST (bottom layer)
    window.draw(m_sprite);

    // Draw the collision shape ON TOP (for debugging)
    window.draw(m_shape);

    // Draw attack hitbox only if attacking
    if (m_isAttacking)
    {
        window.draw(m_attackHitbox);
    }
}

void Player::UnlockDoubleJump()
{
    m_hasDoubleJumpUnlocked = true;
}

sf::Vector2f Player::GetPosition() const
{
    return m_position;
}

sf::FloatRect Player::GetAttackBounds() const
{
    if (m_isAttacking) return m_attackHitbox.getGlobalBounds();
    return sf::FloatRect(); // Return empty rect if not attacking
}

bool Player::IsAttacking() const
{
    return m_isAttacking;
}

int Player::GetFacingDirection() const
{
    return m_facingDirection;
}

sf::FloatRect Player::GetBounds() const
{
    return m_shape.getGlobalBounds();
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
                    m_coyoteTimer = COYOTE_TIME;    // Recharge coyote time
                    m_canDoubleJump = true;         // Recharge double jump
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