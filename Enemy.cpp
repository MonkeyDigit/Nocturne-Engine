#include "Enemy.h"
#include <cmath>

Enemy::Enemy(float startX, float startY)
    : m_position(startX, startY),
    m_velocity(0.0f, 0.0f),
    m_hp(5), // Dies after 5 hits
    m_invulnerabilityTimer(0.0f)
{
    m_shape.setSize({ 32.0f, 32.0f }); // A bit shorter than the player
    m_shape.setOrigin({ 16.0f, 32.0f }); // Bottom-center origin
    m_shape.setFillColor(sf::Color::Red);
    m_shape.setPosition(m_position);
}

void Enemy::Update(sf::Time deltaTime, const Map& map)
{
    // If dead, we could just return here (or handle death state)
    if (m_hp <= 0) return;

    float dt = deltaTime.asSeconds();

    // --- INVULNERABILITY & FLASHING ---
    if (m_invulnerabilityTimer > 0.0f)
    {
        m_invulnerabilityTimer -= dt;

        // Flash white every 0.1 seconds
        if (std::fmod(m_invulnerabilityTimer, 0.1f) > 0.05f)
            m_shape.setFillColor(sf::Color::White);
        else
            m_shape.setFillColor(sf::Color::Red);
    }
    else
    {
        m_shape.setFillColor(sf::Color::Red);
    }

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
}

void Enemy::Draw(sf::RenderWindow& window)
{
    if (m_hp > 0)
    {
        window.draw(m_shape);
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