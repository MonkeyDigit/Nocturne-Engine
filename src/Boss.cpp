#include "Boss.h"
#include <cmath>

Boss::Boss(float startX, float startY)
    : m_position(startX, startY),
    m_velocity(0.0f, 0.0f),
    m_hp(15), // 15 hits to kill
    m_invulnerabilityTimer(0.0f),
    m_currentState(State::Idle),
    m_stateTimer(2.0f), // Start by idling for 2 seconds
    m_facingDirection(1)
{
    m_shape.setSize({ 32.0f, 48.0f }); // Big boss size
    m_shape.setOrigin({ 16.0f, 48.0f }); // Bottom-center origin
    m_shape.setFillColor(sf::Color(150, 0, 200)); // Purple color
    m_shape.setPosition(m_position);
}

void Boss::Update(sf::Time deltaTime, const Map& map, sf::Vector2f playerPos)
{
    if (m_hp <= 0) return;

    float dt = deltaTime.asSeconds();

    // --- AI STATE MACHINE (2 PATTERNS) ---
    // Only process AI if not being knocked back (to avoid overriding knockback velocity)
    if (m_invulnerabilityTimer <= 0.0f)
    {
        if (m_currentState == State::Idle)
        {
            m_stateTimer -= dt;
            if (m_stateTimer <= 0.0f)
            {
                // Transition to Charge
                m_currentState = State::Charge;
                m_stateTimer = 0.8f; // Charge duration

                // Determine which direction to charge based on player position
                m_facingDirection = (playerPos.x > m_position.x) ? 1 : -1;
            }
        }
        else if (m_currentState == State::Charge)
        {
            m_stateTimer -= dt;

            // Apply charge speed
            m_velocity.x = CHARGE_SPEED * m_facingDirection;

            if (m_stateTimer <= 0.0f)
            {
                // Transition back to Idle
                m_currentState = State::Idle;
                m_stateTimer = 2.0f; // Rest duration
            }
        }
    }

    // --- INVULNERABILITY & FLASHING ---
    if (m_invulnerabilityTimer > 0.0f)
    {
        m_invulnerabilityTimer -= dt;
        if (std::fmod(m_invulnerabilityTimer, 0.1f) > 0.05f)
            m_shape.setFillColor(sf::Color::White);
        else
            m_shape.setFillColor(sf::Color(150, 0, 200));
    }
    else
    {
        m_shape.setFillColor(sf::Color(150, 0, 200));
    }

    // --- HORIZONTAL FRICTION ---
    // Apply friction only when Idle or when recovering from a hit
    if (m_currentState == State::Idle || m_invulnerabilityTimer > 0.0f)
    {
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
    }

    // --- GRAVITY & APPLY VELOCITY ---
    m_velocity.y += GRAVITY * dt;

    m_position.x += m_velocity.x * dt;
    m_shape.setPosition(m_position);
    ResolveCollisionsX(map);

    m_position.y += m_velocity.y * dt;
    m_shape.setPosition(m_position);
    ResolveCollisionsY(map);
}

void Boss::Draw(sf::RenderWindow& window)
{
    if (m_hp > 0) window.draw(m_shape);
}

sf::FloatRect Boss::GetBounds() const { return m_shape.getGlobalBounds(); }

bool Boss::IsDead() const { return m_hp <= 0; }

void Boss::TakeDamage(int damage, float knockbackDirectionX)
{
    if (m_invulnerabilityTimer > 0.0f || m_hp <= 0) return;

    m_hp -= damage;
    m_invulnerabilityTimer = INVULNERABILITY_DURATION;

    // Boss gets pushed back slightly and interrupted
    m_velocity.x = KNOCKBACK_FORCE_X * knockbackDirectionX;
    m_velocity.y = -KNOCKBACK_FORCE_Y;

    // Reset AI to Idle when hit to create an "interrupt" feel
    m_currentState = State::Idle;
    m_stateTimer = 1.0f; // Recovers after 1 second
}

// --- COLLISIONS (Identical physics) ---
void Boss::ResolveCollisionsX(const Map& map)
{
    sf::FloatRect bounds = m_shape.getGlobalBounds();
    int leftTile = bounds.position.x / Map::TILE_SIZE;
    int rightTile = (bounds.position.x + bounds.size.x) / Map::TILE_SIZE;
    int topTile = bounds.position.y / Map::TILE_SIZE;
    int bottomTile = (bounds.position.y + bounds.size.y - 0.1f) / Map::TILE_SIZE;

    for (int y = topTile; y <= bottomTile; ++y) {
        for (int x = leftTile; x <= rightTile; ++x) {
            if (map.IsSolid(x, y)) {
                if (m_velocity.x > 0.0f) {
                    m_position.x = x * Map::TILE_SIZE - (bounds.size.x / 2.0f);
                    m_velocity.x = 0.0f; return;
                }
                else if (m_velocity.x < 0.0f) {
                    m_position.x = (x + 1) * Map::TILE_SIZE + (bounds.size.x / 2.0f);
                    m_velocity.x = 0.0f; return;
                }
            }
        }
    }
}

void Boss::ResolveCollisionsY(const Map& map)
{
    sf::FloatRect bounds = m_shape.getGlobalBounds();
    int leftTile = (bounds.position.x + 0.1f) / Map::TILE_SIZE;
    int rightTile = (bounds.position.x + bounds.size.x - 0.1f) / Map::TILE_SIZE;
    int topTile = bounds.position.y / Map::TILE_SIZE;
    int bottomTile = (bounds.position.y + bounds.size.y) / Map::TILE_SIZE;

    for (int y = topTile; y <= bottomTile; ++y) {
        for (int x = leftTile; x <= rightTile; ++x) {
            if (map.IsSolid(x, y)) {
                if (m_velocity.y > 0.0f) {
                    m_position.y = y * Map::TILE_SIZE; m_velocity.y = 0.0f; return;
                }
                else if (m_velocity.y < 0.0f) {
                    m_position.y = (y + 1) * Map::TILE_SIZE + bounds.size.y; m_velocity.y = 0.0f; return;
                }
            }
        }
    }
}