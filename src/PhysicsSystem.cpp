#include <cmath>

#include "PhysicsSystem.h"
#include "Map.h"
#include "EntityManager.h"
#include "EntityBase.h"
#include "CTransform.h"
#include "CBoxCollider.h"
#include "CState.h"

namespace
{
    constexpr float kMapKillZoneTilesBelowMap = 4.0f;
    constexpr float kFallbackFixedDt = 1.0f / 60.0f;

    inline bool IsFiniteVec2(const sf::Vector2f& v)
    {
        return std::isfinite(v.x) && std::isfinite(v.y);
    }

    inline bool IsFiniteRect(const sf::FloatRect& r)
    {
        return IsFiniteVec2(r.position) && IsFiniteVec2(r.size);
    }

    inline float SanitizeDeltaTime(float deltaTime)
    {
        return (std::isfinite(deltaTime) && deltaTime > 0.0f)
            ? deltaTime
            : kFallbackFixedDt;
    }

    inline void ClearColliderContacts(CBoxCollider& collider)
    {
        collider.SetCollidingX(false);
        collider.SetCollidingY(false);
        collider.SetReferenceTile(nullptr);
        collider.m_collisions.clear();
    }
}

void PhysicsSystem::Update(EntityManager& entityManager, Map* gameMap, float deltaTime)
{
    if (!gameMap) return;

    const float frameDt = SanitizeDeltaTime(deltaTime);

    for (auto& entityPair : entityManager.GetEntities())
    {
        EntityBase* entity = entityPair.second.get();

        CTransform* transform = entity->GetComponent<CTransform>();
        CBoxCollider* collider = entity->GetComponent<CBoxCollider>();

        if (!transform || !collider) continue;

        if (!IsFiniteVec2(transform->GetPosition()) || !IsFiniteVec2(transform->GetSize()))
        {
            // Invalid transform cannot produce stable AABB/collision data
            ClearColliderContacts(*collider);

            if (entity->GetType() == EntityType::Projectile)
                entity->DestroyAndDisableProjectileDamage();

            continue;
        }

        ApplyGravityAndMovement(entity, gameMap, frameDt);

        if (!ConstrainToMapBounds(entity, gameMap))
        {
            // Entity is leaving play-space and should not be resolved further this frame
            ClearColliderContacts(*collider);
            continue;
        }

        collider->SetCollidingX(false);
        collider->SetCollidingY(false);

        CheckMapCollisions(entity, gameMap);
        ResolveMapCollisions(entity, gameMap, frameDt);
    }
}

void PhysicsSystem::ApplyGravityAndMovement(EntityBase* entity, Map* map, float deltaTime)
{
    CTransform* transform = entity->GetComponent<CTransform>();
    CBoxCollider* collider = entity->GetComponent<CBoxCollider>();

    if (!IsFiniteVec2(transform->GetVelocity()))
        transform->SetVelocity(0.0f, 0.0f);

    if (!IsFiniteVec2(transform->GetAcceleration()))
        transform->SetAcceleration(0.0f, 0.0f);

    const float gravityRaw = map->GetGravity();
    const float gravity = std::isfinite(gravityRaw) ? gravityRaw : 0.0f;
    transform->AddAcceleration(0.0f, gravity);

    const sf::Vector2f acceleration = transform->GetAcceleration();
    transform->AddVelocity(
        acceleration.x * deltaTime,
        acceleration.y * deltaTime);

    sf::Vector2f frictionValue;
    TileInfo* refTile = collider->GetReferenceTile();

    if (refTile)
    {
        frictionValue = refTile->friction;

        // Some entities (e.g. projectiles) do not have CState
        if (refTile->deadly)
        {
            if (CState* state = entity->GetComponent<CState>())
                state->SetState(EntityState::Dying);
        }
    }
    else if (map->GetDefaultTile())
    {
        frictionValue = map->GetDefaultTile()->friction;
    }
    else
    {
        frictionValue = transform->GetFriction();
    }

    if (!IsFiniteVec2(frictionValue))
        frictionValue = { 0.0f, 0.0f };

    sf::Vector2f speed = transform->GetSpeed();
    if (!IsFiniteVec2(speed))
        speed = { 0.0f, 0.0f };

    const float friction_x = (speed.x * frictionValue.x) * deltaTime;
    const float friction_y = (speed.y * frictionValue.y) * deltaTime;

    sf::Vector2f velocity = transform->GetVelocity();
    if (transform->GetAcceleration().x == 0.0f)
    {
        if (std::abs(velocity.x) - std::abs(friction_x) < 0.0f) velocity.x = 0.0f;
        else velocity.x += (velocity.x < 0.0f) ? friction_x : -friction_x;
    }
    if (velocity.y != 0.0f)
    {
        if (std::abs(velocity.y) - std::abs(friction_y) < 0.0f) velocity.y = 0.0f;
        else velocity.y += (velocity.y < 0.0f) ? friction_y : -friction_y;
    }

    if (!IsFiniteVec2(velocity))
        velocity = { 0.0f, 0.0f };

    transform->SetVelocity(velocity.x, velocity.y);
    transform->SetAcceleration(0.0f, 0.0f);

    sf::Vector2f safeVelocity = transform->GetVelocity();
    if (!IsFiniteVec2(safeVelocity))
    {
        safeVelocity = { 0.0f, 0.0f };
        transform->SetVelocity(0.0f, 0.0f);
    }

    const sf::Vector2f deltaPos = safeVelocity * deltaTime;
    transform->AddPosition(deltaPos.x, deltaPos.y);

    const sf::Vector2f pos = transform->GetPosition();
    const sf::Vector2f size = transform->GetSize();

    collider->SetAABB(sf::FloatRect(
        { pos.x - (size.x * 0.5f), pos.y - size.y },
        { size.x, size.y }
    ));
}

bool PhysicsSystem::ConstrainToMapBounds(EntityBase* entity, Map* map)
{
    CTransform* transform = entity->GetComponent<CTransform>();
    CBoxCollider* collider = entity->GetComponent<CBoxCollider>();

    const sf::Vector2u mapSize = map->GetMapSize();
    const float tileSize = static_cast<float>(map->GetTileSize());

    if (!std::isfinite(tileSize) || tileSize <= 0.0f)
        return false;

    const sf::FloatRect& aabb = collider->GetAABB();
    if (!IsFiniteRect(aabb))
        return false;

    sf::Vector2f position = transform->GetPosition();
    if (!IsFiniteVec2(position))
        return false;

    if (position.x - aabb.size.x * 0.5f < 0.0f)
    {
        position.x = aabb.size.x * 0.5f;
        transform->SetPosition(position.x, position.y);
        transform->SetVelocity(0.0f, transform->GetVelocity().y);
    }
    else if (position.x + aabb.size.x * 0.5f > mapSize.x * tileSize)
    {
        position.x = mapSize.x * tileSize - aabb.size.x * 0.5f;
        transform->SetPosition(position.x, position.y);
        transform->SetVelocity(0.0f, transform->GetVelocity().y);
    }

    if (position.y < 0.0f)
    {
        position.y = 0.0f;
        transform->SetPosition(position.x, position.y);
        transform->SetVelocity(transform->GetVelocity().x, 0.0f);
    }
    else if (position.y >
        (static_cast<float>(mapSize.y) + kMapKillZoneTilesBelowMap) * tileSize)
    {
        if (CState* state = entity->GetComponent<CState>())
        {
            state->SetState(EntityState::Dying);
            transform->SetVelocity(transform->GetVelocity().x, 0.0f);
        }
        else
        {
            if (entity->GetType() == EntityType::Projectile)
            {
                // Ensure projectile is ignored by CombatSystem in this same frame
                entity->DestroyAndDisableProjectileDamage();
            }
            else
            {
                entity->Destroy();
            }
        }

        return false;
    }

    return true;
}