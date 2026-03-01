#include "EntityBase.h"
#include "SharedContext.h"
#include "EntityManager.h"
#include <cmath>
#include <algorithm>
#include <iostream>

bool SortCollisions(const CollisionElement& e1, const CollisionElement& e2)
{
    return e1.m_area > e2.m_area;
}

EntityBase::EntityBase(EntityManager& entityManager)
    : m_entityManager(entityManager), m_name("BaseEntity"),
    m_type(EntityType::Base), m_referenceTile(nullptr),
    m_state(EntityState::Idle), m_id(0),
    m_collidingOnX(false), m_collidingOnY(false),
    m_jumping(false)
{
    // Add the Transform component and store the shortcut pointer
    m_transform = AddComponent<CTransform>();
}

EntityBase::~EntityBase() {}

void EntityBase::SetState(EntityState state)
{
    if (m_state == EntityState::Dying) return;
    m_state = state;
}

void EntityBase::Move(float x, float y)
{
    // TODO: COSA FARE DI QUESTO ???
    //m_positionOld = GetPosition();
    AddPosition(x, y);

    sf::Vector2u mapSize = m_entityManager.GetContext().m_gameMap->GetMapSize();
    float tileSize = static_cast<float>(m_entityManager.GetContext().m_gameMap->GetTileSize());

    // Clamp to window vertical borders
    if (GetPosition().x - m_AABB.size.x * 0.5f < 0.0f)
    {
        SetPosition(m_AABB.size.x * 0.5f, GetPosition().y);
        SetVelocity(0.0f, GetVelocity().y);
    }
    else if (GetPosition().x + m_AABB.size.x * 0.5f > mapSize.x * tileSize)
    {
        SetPosition(mapSize.x * tileSize - m_AABB.size.x * 0.5f, GetPosition().y);
        SetVelocity(0.0f, GetVelocity().y);
    }

    // Clamp to window horizontal borders
    if (GetPosition().y < 0.0f)
    {
        SetPosition(GetPosition().x, 0.0f);
        SetVelocity(GetVelocity().x, 0.0f);
    }
    else if (GetPosition().y > (mapSize.y + 4) * tileSize)
    {
        SetState(EntityState::Dying);
        SetVelocity(GetVelocity().x, 0.0f);
    }

    UpdateAABB();
}

void EntityBase::ApplyFriction(float x, float y)
{
    if (GetAcceleration().x == 0.0f)
    {
        if (GetVelocity().x != 0.0f)
        {
            if (std::abs(GetVelocity().x) - std::abs(x) < 0.0f)
                SetVelocity(0.0f, GetVelocity().y);
            else
            {
                if (GetVelocity().x < 0.0f)
                    AddVelocity(x, 0.0f);
                else
                    AddVelocity(-x, 0.0f);
            }
        }
    }

    if (GetVelocity().y != 0.0f)
    {
        if (std::abs(GetVelocity().y) - std::abs(y) < 0.0f) 
            SetVelocity(GetVelocity().x, 0.0f);
        else
        {
            if (GetVelocity().y < 0.0f) 
                AddVelocity(0.0f, y);
            else
                AddVelocity(0.0f, -y);
        }
    }
}

void EntityBase::Update(float deltaTime)
{
    Map* map = m_entityManager.GetContext().m_gameMap;
    float gravity = map->GetGravity();

    if (!m_jumping) AddAcceleration(0.0f, gravity);
    else m_jumping = false;

    AddVelocity(GetAcceleration().x * deltaTime, GetAcceleration().y * deltaTime);

    sf::Vector2f frictionValue;
    if (m_referenceTile)
    {
        frictionValue = m_referenceTile->friction;
        if (m_referenceTile->deadly) SetState(EntityState::Dying);
    }
    else if (map->GetDefaultTile())
    {
        frictionValue = map->GetDefaultTile()->friction;
    }
    else
    {
        frictionValue = GetFriction();
    }

    float friction_x = (GetSpeed().x * frictionValue.x) * deltaTime;
    float friction_y = (GetSpeed().y * frictionValue.y) * deltaTime;

    ApplyFriction(friction_x, friction_y);
    SetAcceleration(0.0f, 0.0f);

    sf::Vector2f deltaPos = GetVelocity() * deltaTime;
    Move(deltaPos.x, deltaPos.y);

    m_collidingOnX = false;
    m_collidingOnY = false;

    CheckCollisions();
    ResolveCollisions();
}

void EntityBase::UpdateAABB()
{
    // SFML 3: FloatRect requires position vector and size vector
    m_AABB = sf::FloatRect(
        { GetPosition().x - (GetSize().x * 0.5f), GetPosition().y - GetSize().y},
        { GetSize().x, GetSize().y}
    );
}

void EntityBase::CheckCollisions()
{
    Map* gameMap = m_entityManager.GetContext().m_gameMap;
    unsigned int tileSize = gameMap->GetTileSize();

    // Note: in SFML 3, left and top are position.x and position.y, width and height are size.x and size.y
    int fromX = std::floor(m_AABB.position.x / tileSize);
    int toX = std::floor((m_AABB.position.x + m_AABB.size.x) / tileSize);

    int fromY = std::floor(m_AABB.position.y / tileSize);
    int toY = std::floor((m_AABB.position.y + m_AABB.size.y) / tileSize);

    for (int x = fromX; x <= toX; ++x)
    {
        for (int y = fromY; y <= toY; ++y)
        {
            Tile* tile = gameMap->GetTile(x, y);
            if (!tile) continue;

            sf::FloatRect tileBounds(
                { static_cast<float>(x * tileSize), static_cast<float>(y * tileSize) },
                { static_cast<float>(tileSize), static_cast<float>(tileSize) }
            );

            // SFML 3 intersection check
            std::optional<sf::FloatRect> intersection = m_AABB.findIntersection(tileBounds);
            if (intersection.has_value())
            {
                float area = intersection->size.x * intersection->size.y;
                CollisionElement e(area, tile->properties, tileBounds);
                m_collisions.emplace_back(e);

                if (tile->warp && m_type == EntityType::Player)
                {
                    gameMap->LoadNext();
                }
            }
        }
    }
}

void EntityBase::ResolveCollisions()
{
    if (!m_collisions.empty())
    {
        std::sort(m_collisions.begin(), m_collisions.end(), SortCollisions);
        Map* gameMap = m_entityManager.GetContext().m_gameMap;
        float tileSize = static_cast<float>(gameMap->GetTileSize());

        for (auto& itr : m_collisions)
        {
            std::optional<sf::FloatRect> intersection = m_AABB.findIntersection(itr.m_tileBounds);
            if (!intersection.has_value()) continue;

            sf::Vector2f delta;
            delta.x = (m_AABB.position.x + (m_AABB.size.x * 0.5f)) - (itr.m_tileBounds.position.x + (itr.m_tileBounds.size.x * 0.5f));
            delta.y = (m_AABB.position.y + (m_AABB.size.y * 0.5f)) - (itr.m_tileBounds.position.y + (itr.m_tileBounds.size.y * 0.5f));

            sf::Vector2f deltaDiff;
            deltaDiff.x = std::abs(delta.x) - (m_AABB.size.x * 0.5f + itr.m_tileBounds.size.x * 0.5f);
            deltaDiff.y = std::abs(delta.y) - (m_AABB.size.y * 0.5f + itr.m_tileBounds.size.y * 0.5f);

            double resolve = 0.0;

            // Push to the side
            if (deltaDiff.x > deltaDiff.y)
            {
                // Push to the right
                if (delta.x > 0.0f) resolve = (itr.m_tileBounds.position.x + tileSize) - m_AABB.position.x;
                else resolve = -(((double)m_AABB.position.x + (double)m_AABB.size.x) - (double)itr.m_tileBounds.position.x);

                // Prevent killing velocity when going away from the tile
                if ((delta.x > 0.0f && GetVelocity().x < 0.0f) || (delta.x < 0.0f && GetVelocity().x > 0.0f))
                    SetVelocity(0.0f, GetVelocity().y);

                Move(static_cast<float>(resolve), 0.0f);
                m_collidingOnX = true;
            }
            else // Push to top or bottom
            {
                if (delta.y > 0.0f) resolve = (itr.m_tileBounds.position.y + tileSize) - m_AABB.position.y;
                else resolve = -((m_AABB.position.y + m_AABB.size.y) - itr.m_tileBounds.position.y);

                if ((delta.y > 0.0f && GetVelocity().y < 0.0f) || (delta.y < 0.0f && GetVelocity().y > 0.0f))
                    SetVelocity(GetVelocity().x, 0.0f);

                Move(0.0f, static_cast<float>(resolve));

                if (m_collidingOnY) continue;

                if (delta.y < 0.0f) m_referenceTile = itr.m_tile;
                else m_referenceTile = nullptr;

                m_collidingOnY = true;
            }
        }
        m_collisions.clear();
    }

    if (!m_collidingOnY) m_referenceTile = nullptr;
}

EntityType EntityBase::GetType() const { return m_type; }
std::string EntityBase::GetName() const { return m_name; }
unsigned int EntityBase::GetId() const { return m_id; }
EntityState EntityBase::GetState() const { return m_state; }