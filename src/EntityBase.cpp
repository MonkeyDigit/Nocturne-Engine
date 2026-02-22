#include "EntityBase.h"
#include "EntityManager.h"
#include "SharedContext.h"
#include "Map.h"
#include <cmath>
#include <algorithm>

EntityBase::EntityBase(EntityManager& entityManager)
    : m_entityManager(entityManager),
    m_name("BaseEntity"),
    m_type(EntityType::Base),
    m_id(0),
    m_state(EntityState::Idle),
    m_isGrounded(false),
    m_collidingOnX(false),
    m_collidingOnY(false)
    // TODO: Perché non vengono inizializzati tutti i membri ?
{}

void EntityBase::SetPosition(float x, float y) { m_position = sf::Vector2f(x, y); UpdateAABB(); }
void EntityBase::SetPosition(const sf::Vector2f& pos) { m_position = pos; UpdateAABB(); }
void EntityBase::SetSize(float x, float y) { m_size = sf::Vector2f(x, y); UpdateAABB(); }
void EntityBase::SetState(EntityState state) { m_state = state; }

void EntityBase::AddVelocity(float x, float y) { m_velocity += sf::Vector2f(x, y); }
void EntityBase::Accelerate(float x, float y) { m_acceleration += sf::Vector2f(x, y); }
void EntityBase::SetAcceleration(float x, float y) { m_acceleration = sf::Vector2f(x, y); }
void EntityBase::SetVelocity(float x, float y) { m_velocity = sf::Vector2f(x, y); }

unsigned int EntityBase::GetId() const { return m_id; }
std::string EntityBase::GetName() const { return m_name; }
EntityType EntityBase::GetType() const { return m_type; }
EntityState EntityBase::GetState() const { return m_state; }
sf::Vector2f EntityBase::GetPosition() const { return m_position; }
sf::Vector2f EntityBase::GetSize() const { return m_size; }

void EntityBase::ApplyFriction(float x, float y)
{
    if (m_velocity.x != 0.0f)
    {
        if (std::abs(m_velocity.x) - std::abs(x) < 0.0f) m_velocity.x = 0.0f;
        else m_velocity.x += (m_velocity.x > 0.0f ? -x : x);
    }
    if (m_velocity.y != 0.0f)
    {
        if (std::abs(m_velocity.y) - std::abs(y) < 0.0f) m_velocity.y = 0.0f;
        else m_velocity.y += (m_velocity.y > 0.0f ? -y : y);
    }
}

void EntityBase::Update(float deltaTime)
{
    m_positionOld = m_position;

    // Apply acceleration and velocity
    m_velocity += m_acceleration * deltaTime;

    if (m_acceleration.x == 0.0f) ApplyFriction(m_friction.x * deltaTime, 0.0f);
    if (m_acceleration.y == 0.0f) ApplyFriction(0.0f, m_friction.y * deltaTime);

    // Clamp velocity
    m_velocity.x = std::clamp(m_velocity.x, -m_maxVelocity.x, m_maxVelocity.x);
    m_velocity.y = std::clamp(m_velocity.y, -m_maxVelocity.y, m_maxVelocity.y);

    // X Axis Move & Collide
    m_position.x += m_velocity.x * deltaTime;
    UpdateAABB();
    ResolveMapCollisionsX();

    // Y Axis Move & Collide
    m_position.y += m_velocity.y * deltaTime;
    UpdateAABB();
    ResolveMapCollisionsY();
}

void EntityBase::UpdateAABB()
{
    // Origin is at bottom-center. SFML 3 FloatRect takes position (top-left) and size.
    m_AABB = sf::FloatRect(
        { m_position.x - (m_size.x / 2.0f), m_position.y - m_size.y },
        m_size
    );
}

void EntityBase::ResolveMapCollisionsX()
{
    Map* gameMap = m_entityManager.GetContext().m_gameMap;
    if (!gameMap) return;

    float tileSize = 16.0f; // TODO: No magic number
    int leftTile = m_AABB.position.x / tileSize;
    int rightTile = (m_AABB.position.x + m_AABB.size.x) / tileSize;
    int topTile = m_AABB.position.y / tileSize;
    int bottomTile = (m_AABB.position.y + m_AABB.size.y - 0.1f) / tileSize;

    m_collidingOnX = false;

    for (int y = topTile; y <= bottomTile; ++y)
    {
        for (int x = leftTile; x <= rightTile; ++x)
        {
            if (gameMap->IsSolid(x, y))
            {
                if (m_velocity.x > 0.0f)
                {
                    m_position.x = x * tileSize - (m_AABB.size.x / 2.0f);
                    m_velocity.x = 0.0f;
                    m_collidingOnX = true;
                    UpdateAABB();
                    return;
                }
                else if (m_velocity.x < 0.0f)
                {
                    m_position.x = (x + 1) * tileSize + (m_AABB.size.x / 2.0f);
                    m_velocity.x = 0.0f;
                    m_collidingOnX = true;
                    UpdateAABB();
                    return;
                }
            }
        }
    }
}

void EntityBase::ResolveMapCollisionsY()
{
    Map* gameMap = m_entityManager.GetContext().m_gameMap;
    if (!gameMap) return;

    float tileSize = 16.0f;
    int leftTile = (m_AABB.position.x + 0.1f) / tileSize;
    int rightTile = (m_AABB.position.x + m_AABB.size.x - 0.1f) / tileSize;
    int topTile = m_AABB.position.y / tileSize;
    int bottomTile = (m_AABB.position.y + m_AABB.size.y) / tileSize;

    m_isGrounded = false;
    m_collidingOnY = false;

    for (int y = topTile; y <= bottomTile; ++y)
    {
        for (int x = leftTile; x <= rightTile; ++x)
        {
            if (gameMap->IsSolid(x, y))
            {
                if (m_velocity.y > 0.0f)
                {
                    m_position.y = y * tileSize;
                    m_velocity.y = 0.0f;
                    m_isGrounded = true;
                    m_collidingOnY = true;
                    UpdateAABB();
                    return;
                }
                else if (m_velocity.y < 0.0f)
                {
                    m_position.y = (y + 1) * tileSize + m_AABB.size.y;
                    m_velocity.y = 0.0f;
                    m_collidingOnY = true;
                    UpdateAABB();
                    return;
                }
            }
        }
    }
}