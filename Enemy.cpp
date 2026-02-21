#include <iostream>
#include <cmath>
#include "Enemy.h"

Enemy::Enemy(float startX, float startY)
    : m_sprite(m_texture), 
    m_position(startX, startY),
    m_velocity(0.0f, 0.0f),
    m_hp(5), // Dies after 5 hits
    m_invulnerabilityTimer(0.0f),
    m_facingDirection(-1),
    m_animState(AnimState::Walk),
    m_animTimer(0.0f),
    m_currentFrame(0)
{
    m_shape.setSize({ 20.0f, 26.0f });
    m_shape.setOrigin({ 10.0f, 26.0f }); // Bottom-center origin
    // Debug Hitbox
    m_shape.setFillColor(sf::Color(255, 0, 0, 80));
    m_shape.setOutlineThickness(-1.0f);
    m_shape.setOutlineColor(sf::Color::Red);

    // Load Texture
    if (!m_texture.loadFromFile("media/textures/RatSheet.png"))
    {
        std::cerr << "ERROR: Could not load RatSheet.png!\n";
    }

    m_sprite.setOrigin({ SPRITE_WIDTH / 2.0f, (float)SPRITE_HEIGHT });
    m_sprite.setTextureRect(sf::IntRect({ 0, 0 }, { SPRITE_WIDTH, SPRITE_HEIGHT }));
}

void Enemy::Update(sf::Time deltaTime, const Map& map)
{
    // If dead, we could just return here (or handle death state)
    if (m_hp <= 0) return;

    float dt = deltaTime.asSeconds();

    // --- INVULNERABILITY & FLASHING ---
    if (m_invulnerabilityTimer > 0.0f)
        m_invulnerabilityTimer -= dt;

    // --- HORIZONTAL FRICTION ---
    // Slow down the enemy if they are sliding from a knockback
    if (m_velocity.x > 0.0f)
    {
        m_velocity.x -= FRICTION * dt;
        if (m_velocity.x < 0.0f) m_velocity.x = 0.0f;
    }
    else if (m_velocity.x < 0.0f)
    {
        m_velocity.x += FRICTION * dt;
        if (m_velocity.x > 0.0f) m_velocity.x = 0.0f;
    }

    // --- GRAVITY ---
    m_velocity.y += GRAVITY * dt;

    // --- APPLY VELOCITY & COLLISIONS ---
    m_position.x += m_velocity.x * dt;
    m_shape.setPosition(m_position);
    ResolveCollisionsX(map);

    m_position.y += m_velocity.y * dt;
    m_shape.setPosition(m_position);
    ResolveCollisionsY(map);

    // Sync visual shape with logical position
    m_shape.setPosition(m_position);

    // ==========================================
    // VISUAL ANIMATION LOGIC
    // ==========================================

    // Update direction based on movement (or knockback)
    if (m_velocity.x > 0.1f) m_facingDirection = 1;
    else if (m_velocity.x < -0.1f) m_facingDirection = -1;

    // Determine the current logical state
    AnimState nextState = AnimState::Idle;

    /*
    if (m_isAttacking)
        nextState = AnimState::Attack;
    else if (!m_isGrounded)
    {
        if (m_velocity.y < 0.0f) nextState = AnimState::Jump;
        else nextState = AnimState::Fall;
    }
    else if (std::abs(m_velocity.x) > 10.0f)
        nextState = AnimState::Run;
    */

    // Mapping the states
    int row = 0;
    int startFrame = 0;
    int endFrame = 0;
    float frameTime = 0.06f;
    bool loop = true;

    if (nextState == AnimState::Idle) {
        row = 0; startFrame = 0; endFrame = 8; frameTime = 0.2f; loop = true;
    }
    else if (nextState == AnimState::Walk) {
        row = 3; startFrame = 0; endFrame = 4; frameTime = 0.1f; loop = true;
    }
    else if (nextState == AnimState::Attack) {
        row = 2; startFrame = 0; endFrame = 5; frameTime = 0.08f; loop = false;
    }
    else if (nextState == AnimState::Hurt) {
        row = 5; startFrame = 0; endFrame = 2; frameTime = 0.2f; loop = false;
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

    // Damage flicker
    if (m_invulnerabilityTimer > 0.0f && std::fmod(m_invulnerabilityTimer, 0.1f) > 0.05f)
        m_sprite.setColor(sf::Color(255, 100, 100));
    else
        m_sprite.setColor(sf::Color::White);

    // Synchronize the frame to logical position
    m_sprite.setPosition(m_position);

    // Turn the frame left or right
    if (m_facingDirection == 1)
        m_sprite.setScale({ -1.0f, 1.0f });
    else
        m_sprite.setScale({ 1.0f, 1.0f });
}

void Enemy::Draw(sf::RenderWindow& window)
{
    if (m_hp > 0)
    {
        window.draw(m_sprite);
        window.draw(m_shape); // Debug
    }
}

sf::FloatRect Enemy::GetBounds() const
{
    return m_shape.getGlobalBounds();
}

void Enemy::TakeDamage(int damage, float knockbackDirectionX)
{
    // Ignore damage if still invulnerable or already dead
    if (m_invulnerabilityTimer > 0.0f || m_hp <= 0) return;

    m_hp -= damage;
    m_invulnerabilityTimer = INVULNERABILITY_DURATION;

    // Apply knockback (push back and slightly up)
    m_velocity.x = KNOCKBACK_FORCE_X * knockbackDirectionX;
    m_velocity.y = -KNOCKBACK_FORCE_Y;
}

// --- COLLISIONS (Identical to Player physics) ---

void Enemy::ResolveCollisionsX(const Map& map)
{
    sf::FloatRect bounds = m_shape.getGlobalBounds();
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
                if (m_velocity.x > 0.0f)
                {
                    m_position.x = x * Map::TILE_SIZE - (bounds.size.x / 2.0f);
                    m_velocity.x = 0.0f;
                    return;
                }
                else if (m_velocity.x < 0.0f)
                {
                    m_position.x = (x + 1) * Map::TILE_SIZE + (bounds.size.x / 2.0f);
                    m_velocity.x = 0.0f;
                    return;
                }
            }
        }
    }
}

void Enemy::ResolveCollisionsY(const Map& map)
{
    sf::FloatRect bounds = m_shape.getGlobalBounds();
    int leftTile = (bounds.position.x + 0.1f) / Map::TILE_SIZE;
    int rightTile = (bounds.position.x + bounds.size.x - 0.1f) / Map::TILE_SIZE;
    int topTile = bounds.position.y / Map::TILE_SIZE;
    int bottomTile = (bounds.position.y + bounds.size.y) / Map::TILE_SIZE;

    for (int y = topTile; y <= bottomTile; ++y)
    {
        for (int x = leftTile; x <= rightTile; ++x)
        {
            if (map.IsSolid(x, y))
            {
                if (m_velocity.y > 0.0f)
                {
                    m_position.y = y * Map::TILE_SIZE;
                    m_velocity.y = 0.0f;
                    return;
                }
                else if (m_velocity.y < 0.0f)
                {
                    m_position.y = (y + 1) * Map::TILE_SIZE + bounds.size.y;
                    m_velocity.y = 0.0f;
                    return;
                }
            }
        }
    }
}