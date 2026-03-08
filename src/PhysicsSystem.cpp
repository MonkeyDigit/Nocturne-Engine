#include <cmath>
#include <algorithm>
#include "PhysicsSystem.h"
#include "EntityManager.h"
#include "CTransform.h"
#include "CBoxCollider.h"
#include "CState.h"

void PhysicsSystem::Update(EntityManager& entityManager, Map* gameMap, float deltaTime)
{
    // Loop through all entities in the game
    for (auto& entityPair : entityManager.GetEntities())
    {
        EntityBase* entity = entityPair.second.get();

        // We only process physics if the entity has BOTH a Transform and a Collider!
        CTransform* transform = entity->GetComponent<CTransform>();
        CBoxCollider* collider = entity->GetComponent<CBoxCollider>();

        if (!transform || !collider) continue;

        // Apply Forces and Move
        ApplyGravityAndMovement(entity, gameMap, deltaTime);

        // Map Boundaries
        ConstrainToMapBounds(entity, gameMap);

        // Reset collision flags for this frame
        collider->SetCollidingX(false);
        collider->SetCollidingY(false);

        // Map Collisions
        CheckMapCollisions(entity, gameMap);
        ResolveMapCollisions(entity, gameMap, deltaTime);
    }
}

void PhysicsSystem::ApplyGravityAndMovement(EntityBase* entity, Map* map, float deltaTime)
{
    CTransform* transform = entity->GetComponent<CTransform>();
    CBoxCollider* collider = entity->GetComponent<CBoxCollider>();

    float gravity = map->GetGravity();

    transform->AddAcceleration(0.0f, gravity);

    transform->AddVelocity(transform->GetAcceleration().x * deltaTime, transform->GetAcceleration().y * deltaTime);

    sf::Vector2f frictionValue;
    TileInfo* refTile = collider->GetReferenceTile();

    if (refTile)
    {
        frictionValue = refTile->friction;

        // Some entities (e.g. projectiles) do not have CState.
        if (refTile->deadly)
        {
            if (CState* state = entity->GetComponent<CState>())
                state->SetState(EntityState::Dying);
        }
    }
    else if (map->GetDefaultTile())
        frictionValue = map->GetDefaultTile()->friction;
    else
        frictionValue = transform->GetFriction();


    float friction_x = (transform->GetSpeed().x * frictionValue.x) * deltaTime;
    float friction_y = (transform->GetSpeed().y * frictionValue.y) * deltaTime;

    // Basic Friction Application (Simplified from your original ApplyFriction)
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

    // MOVE
    sf::Vector2f deltaPos = transform->GetVelocity() * deltaTime;
    transform->AddPosition(deltaPos.x, deltaPos.y);

    // Update the AABB position
    collider->SetAABB(sf::FloatRect(
        { transform->GetPosition().x - (transform->GetSize().x * 0.5f), transform->GetPosition().y - transform->GetSize().y },
        { transform->GetSize().x, transform->GetSize().y }
    ));
}

void PhysicsSystem::ConstrainToMapBounds(EntityBase* entity, Map* map)
{
    CTransform* transform = entity->GetComponent<CTransform>();
    CBoxCollider* collider = entity->GetComponent<CBoxCollider>();

    sf::Vector2u mapSize = map->GetMapSize();
    float tileSize = static_cast<float>(map->GetTileSize());
    const sf::FloatRect& aabb = collider->GetAABB();

    // Clamp X
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

    // Clamp Y
    if (transform->GetPosition().y < 0.0f)
    {
        transform->SetPosition(transform->GetPosition().x, 0.0f);
        transform->SetVelocity(transform->GetVelocity().x, 0.0f);
    }
    else if (transform->GetPosition().y > (mapSize.y + 4) * tileSize)
    {
        // Avoid null dereference on entities without CState.
        if (CState* state = entity->GetComponent<CState>())
            state->SetState(EntityState::Dying);

        transform->SetVelocity(transform->GetVelocity().x, 0.0f);
    }

}

void PhysicsSystem::CheckMapCollisions(EntityBase* entity, Map* map)
{
    CTransform* transform = entity->GetComponent<CTransform>();
    CBoxCollider* collider = entity->GetComponent<CBoxCollider>();

    const unsigned int tileSize = map->GetTileSize();
    if (tileSize == 0u) return;

    const float tileSizeF = static_cast<float>(tileSize);
    const sf::FloatRect& aabb = collider->GetAABB();

    const int fromX = static_cast<int>(std::floor(aabb.position.x / tileSizeF));
    const int toX = static_cast<int>(std::floor((aabb.position.x + aabb.size.x) / tileSizeF));

    const int fromY = static_cast<int>(std::floor(aabb.position.y / tileSizeF));
    const int toY = static_cast<int>(std::floor((aabb.position.y + aabb.size.y) / tileSizeF));

    for (int x = fromX; x <= toX; ++x)
    {
        for (int y = fromY; y <= toY; ++y)
        {
            Tile* tile = map->GetTile(x, y);
            if (!tile) continue;

            sf::FloatRect tileBounds(
                { static_cast<float>(x * tileSize), static_cast<float>(y * tileSize) },
                { static_cast<float>(tileSize), static_cast<float>(tileSize) }
            );

            std::optional<sf::FloatRect> intersection = aabb.findIntersection(tileBounds);
            if (intersection.has_value())
            {
                float area = intersection->size.x * intersection->size.y;
                collider->m_collisions.emplace_back(area, &tile->properties, tileBounds);

                // Handle map changing (warp)
                if (tile->warp && entity->GetType() == EntityType::Player && !map->IsNextMapQueued())
                    map->LoadNext();
            }
        }
    }
}

void PhysicsSystem::ResolveMapCollisions(EntityBase* entity, Map* map, float deltaTime)
{
    CTransform* transform = entity->GetComponent<CTransform>();
    CBoxCollider* collider = entity->GetComponent<CBoxCollider>();

    if (collider->m_collisions.empty()) return;

    // Sort collisions by area (Your brilliant fix for the Tile Seam Bug!)
    std::sort(collider->m_collisions.begin(), collider->m_collisions.end(),
        [](const CollisionElement& e1, const CollisionElement& e2) {
            return e1.m_area > e2.m_area;
        });

    float tileSize = static_cast<float>(map->GetTileSize());

    for (auto& itr : collider->m_collisions)
    {
        const sf::FloatRect& aabb = collider->GetAABB();
        std::optional<sf::FloatRect> intersection = aabb.findIntersection(itr.m_tileBounds);
        if (!intersection.has_value()) continue;

        sf::Vector2f delta;
        delta.x = (aabb.position.x + (aabb.size.x * 0.5f)) - (itr.m_tileBounds.position.x + (itr.m_tileBounds.size.x * 0.5f));
        delta.y = (aabb.position.y + (aabb.size.y * 0.5f)) - (itr.m_tileBounds.position.y + (itr.m_tileBounds.size.y * 0.5f));

        sf::Vector2f deltaDiff;
        deltaDiff.x = std::abs(delta.x) - (aabb.size.x * 0.5f + itr.m_tileBounds.size.x * 0.5f);
        deltaDiff.y = std::abs(delta.y) - (aabb.size.y * 0.5f + itr.m_tileBounds.size.y * 0.5f);

        // --- ONEWAY PLATFORMS LOGIC ---
        if (itr.m_tile->collision == TileCollision::OneWay)
        {
            // Ignore if the player is jumping upwards or is stationary
            if (transform->GetVelocity().y <= 0.0f) continue;

            // Ignore if hitting from BELOW (player center is below tile center)
            if (delta.y > 0.0f) continue;

            // ROBUST LOGIC: Calculate where the feet were in the PREVIOUS FRAME
            // Since the engine uses a fixed timestep of 60 FPS, we can use 1.0f / 60.0f
            // Use the real frame delta for previous-frame reconstruction
            const float frameDt = (deltaTime > 0.0f) ? deltaTime : (1.0f / 60.0f);
            const float moveY = transform->GetVelocity().y * frameDt;
            const float previousBottom = (aabb.position.y + aabb.size.y) - moveY;
            float tileTop = itr.m_tileBounds.position.y;

            // If the feet were ALREADY below the platform's edge in the last frame, 
            // it means we were inside and are falling through it. Ignore
            // (Adding 1.0f as a safe tolerance for floating point calculations)
            if (previousBottom > tileTop + 1.0f) continue;

            // FORCE VERTICAL RESOLUTION
            // By setting deltaDiff.x to a tiny negative value, we fake the condition
            // 'if (deltaDiff.x > deltaDiff.y)'. This FORCES the engine to apply 
            // the upward push (Y) and prevents the player from slipping off tile edges
            deltaDiff.x = -1000.0f;
        }
        double resolve = 0.0;

        // Push to the side
        if (deltaDiff.x > deltaDiff.y)
        {
            if (delta.x > 0.0f) resolve = (itr.m_tileBounds.position.x + tileSize) - aabb.position.x;
            else resolve = -(((double)aabb.position.x + (double)aabb.size.x) - (double)itr.m_tileBounds.position.x);

            if ((delta.x > 0.0f && transform->GetVelocity().x < 0.0f) || (delta.x < 0.0f && transform->GetVelocity().x > 0.0f))
                transform->SetVelocity(0.0f, transform->GetVelocity().y);

            transform->AddPosition(static_cast<float>(resolve), 0.0f);
            collider->SetCollidingX(true);
        }
        else // Push to top or bottom
        {
            if (delta.y > 0.0f) resolve = (itr.m_tileBounds.position.y + tileSize) - aabb.position.y;
            else resolve = -((aabb.position.y + aabb.size.y) - itr.m_tileBounds.position.y);

            if ((delta.y > 0.0f && transform->GetVelocity().y < 0.0f) || (delta.y < 0.0f && transform->GetVelocity().y > 0.0f))
                transform->SetVelocity(transform->GetVelocity().x, 0.0f);

            transform->AddPosition(0.0f, static_cast<float>(resolve));

            if (collider->IsCollidingY()) continue;

            if (delta.y < 0.0f) collider->SetReferenceTile(itr.m_tile);
            else collider->SetReferenceTile(nullptr);

            collider->SetCollidingY(true);
        }

        // Extremely important: Update the AABB immediately after being pushed by a wall!
        collider->SetAABB(sf::FloatRect(
            { transform->GetPosition().x - (transform->GetSize().x * 0.5f), transform->GetPosition().y - transform->GetSize().y },
            { transform->GetSize().x, transform->GetSize().y }
        ));
    }
    collider->m_collisions.clear();

    if (!collider->IsCollidingY()) collider->SetReferenceTile(nullptr);
}