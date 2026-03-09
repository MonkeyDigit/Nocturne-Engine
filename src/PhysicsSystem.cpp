#include <cmath>

#include "PhysicsSystem.h"
#include "Map.h"
#include "EntityManager.h"
#include "EntityBase.h"
#include "CTransform.h"
#include "CBoxCollider.h"
#include "CState.h"
#include "CProjectile.h"

namespace
{
    constexpr float kMapKillZoneTilesBelowMap = 4.0f;
}

void PhysicsSystem::Update(EntityManager& entityManager, Map* gameMap, float deltaTime)
{
    if (!gameMap) return;

    for (auto& entityPair : entityManager.GetEntities())
    {
        EntityBase* entity = entityPair.second.get();

        CTransform* transform = entity->GetComponent<CTransform>();
        CBoxCollider* collider = entity->GetComponent<CBoxCollider>();

        if (!transform || !collider) continue;

        ApplyGravityAndMovement(entity, gameMap, deltaTime);

        if (!ConstrainToMapBounds(entity, gameMap))
        {
            // Entity is leaving play-space and should not be resolved further this frame
            collider->SetCollidingX(false);
            collider->SetCollidingY(false);
            collider->SetReferenceTile(nullptr);
            collider->m_collisions.clear();
            continue;
        }

        collider->SetCollidingX(false);
        collider->SetCollidingY(false);

        CheckMapCollisions(entity, gameMap);
        ResolveMapCollisions(entity, gameMap, deltaTime);
    }
}

void PhysicsSystem::ApplyGravityAndMovement(EntityBase* entity, Map* map, float deltaTime)
{
    CTransform* transform = entity->GetComponent<CTransform>();
    CBoxCollider* collider = entity->GetComponent<CBoxCollider>();

    const float gravity = map->GetGravity();
    transform->AddAcceleration(0.0f, gravity);

    transform->AddVelocity(
        transform->GetAcceleration().x * deltaTime,
        transform->GetAcceleration().y * deltaTime);

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

    const float friction_x = (transform->GetSpeed().x * frictionValue.x) * deltaTime;
    const float friction_y = (transform->GetSpeed().y * frictionValue.y) * deltaTime;

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

    transform->SetVelocity(velocity.x, velocity.y);
    transform->SetAcceleration(0.0f, 0.0f);

    const sf::Vector2f deltaPos = transform->GetVelocity() * deltaTime;
    transform->AddPosition(deltaPos.x, deltaPos.y);

    collider->SetAABB(sf::FloatRect(
        { transform->GetPosition().x - (transform->GetSize().x * 0.5f), transform->GetPosition().y - transform->GetSize().y },
        { transform->GetSize().x, transform->GetSize().y }
    ));
}

bool PhysicsSystem::ConstrainToMapBounds(EntityBase* entity, Map* map)
{
    CTransform* transform = entity->GetComponent<CTransform>();
    CBoxCollider* collider = entity->GetComponent<CBoxCollider>();

    const sf::Vector2u mapSize = map->GetMapSize();
    const float tileSize = static_cast<float>(map->GetTileSize());
    const sf::FloatRect& aabb = collider->GetAABB();

    if (transform->GetPosition().x - aabb.size.x * 0.5f < 0.0f)
    {
        transform->SetPosition(aabb.size.x * 0.5f, transform->GetPosition().y);
        transform->SetVelocity(0.0f, transform->GetVelocity().y);
    }
    else if (transform->GetPosition().x + aabb.size.x * 0.5f > mapSize.x * tileSize)
    {
        transform->SetPosition(mapSize.x * tileSize - aabb.size.x * 0.5f, transform->GetPosition().y);
        transform->SetVelocity(0.0f, transform->GetVelocity().y);
    }

    if (transform->GetPosition().y < 0.0f)
    {
        transform->SetPosition(transform->GetPosition().x, 0.0f);
        transform->SetVelocity(transform->GetVelocity().x, 0.0f);
    }
    else if (transform->GetPosition().y >
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
                entity->RemoveComponent<CProjectile>();
            }

            entity->Destroy();
        }

        return false;
    }

    return true;
}