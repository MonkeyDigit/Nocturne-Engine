#include <algorithm>
#include <cmath>

#include "PhysicsSystem.h"
#include "Map.h"
#include "EntityBase.h"
#include "CTransform.h"
#include "CBoxCollider.h"

namespace
{
    constexpr float kFallbackFixedDt = 1.0f / 60.0f;
    constexpr float kOneWayFootTolerance = 1.0f;
    constexpr float kTileEdgeEpsilon = 0.001f;

    inline bool IsFinite(float value)
    {
        return std::isfinite(value);
    }

    inline bool IsFiniteVec2(const sf::Vector2f& value)
    {
        return IsFinite(value.x) && IsFinite(value.y);
    }

    inline bool IsValidRect(const sf::FloatRect& rect)
    {
        return IsFiniteVec2(rect.position) &&
            IsFiniteVec2(rect.size) &&
            rect.size.x > 0.0f &&
            rect.size.y > 0.0f;
    }

    inline float SanitizeDeltaTime(float deltaTime)
    {
        return (IsFinite(deltaTime) && deltaTime > 0.0f) ? deltaTime : kFallbackFixedDt;
    }

    inline sf::FloatRect BuildEntityAABB(const CTransform& transform)
    {
        const sf::Vector2f pos = transform.GetPosition();
        const sf::Vector2f size = transform.GetSize();

        return sf::FloatRect(
            { pos.x - (size.x * 0.5f), pos.y - size.y },
            { size.x, size.y }
        );
    }
}

void PhysicsSystem::CheckMapCollisions(EntityBase* entity, Map* map)
{
    if (!entity || !map) return;

    CBoxCollider* collider = entity->GetComponent<CBoxCollider>();
    if (!collider) return;

    collider->m_collisions.clear();

    const unsigned int tileSize = map->GetTileSize();
    const sf::Vector2u mapSize = map->GetMapSize();
    if (tileSize == 0u || mapSize.x == 0u || mapSize.y == 0u) return;

    const sf::FloatRect& aabb = collider->GetAABB();
    if (!IsValidRect(aabb)) return;

    const float tileSizeF = static_cast<float>(tileSize);

    const float maxXCoord = aabb.position.x + std::max(0.0f, aabb.size.x - kTileEdgeEpsilon);
    const float maxYCoord = aabb.position.y + std::max(0.0f, aabb.size.y - kTileEdgeEpsilon);

    int fromX = static_cast<int>(std::floor(aabb.position.x / tileSizeF));
    int toX = static_cast<int>(std::floor(maxXCoord / tileSizeF));
    int fromY = static_cast<int>(std::floor(aabb.position.y / tileSizeF));
    int toY = static_cast<int>(std::floor(maxYCoord / tileSizeF));

    const int maxTileX = static_cast<int>(mapSize.x) - 1;
    const int maxTileY = static_cast<int>(mapSize.y) - 1;
    if (maxTileX < 0 || maxTileY < 0) return;

    fromX = std::clamp(fromX, 0, maxTileX);
    toX = std::clamp(toX, 0, maxTileX);
    fromY = std::clamp(fromY, 0, maxTileY);
    toY = std::clamp(toY, 0, maxTileY);

    if (fromX > toX || fromY > toY) return;

    for (int x = fromX; x <= toX; ++x)
    {
        for (int y = fromY; y <= toY; ++y)
        {
            Tile* tile = map->GetTile(x, y);
            if (!tile) continue;

            if (tile->properties.collision == TileCollision::None && !tile->warp)
                continue;

            const sf::FloatRect tileBounds(
                { static_cast<float>(x * tileSize), static_cast<float>(y * tileSize) },
                { tileSizeF, tileSizeF }
            );

            std::optional<sf::FloatRect> intersection = aabb.findIntersection(tileBounds);
            if (!intersection.has_value()) continue;

            const float area = intersection->size.x * intersection->size.y;
            if (!IsFinite(area) || area <= 0.0f) continue;

            if (tile->properties.collision != TileCollision::None)
                collider->m_collisions.emplace_back(area, &tile->properties, tileBounds);

            // Warp trigger stays independent from solidity
            if (tile->warp &&
                entity->GetType() == EntityType::Player &&
                !map->IsNextMapQueued())
            {
                map->LoadNext();
            }
        }
    }
}

void PhysicsSystem::ResolveMapCollisions(EntityBase* entity, Map* map, float deltaTime)
{
    if (!entity || !map) return;

    CTransform* transform = entity->GetComponent<CTransform>();
    CBoxCollider* collider = entity->GetComponent<CBoxCollider>();
    if (!transform || !collider) return;

    if (collider->m_collisions.empty())
    {
        // No contact this frame: do not keep stale ground tile
        collider->SetReferenceTile(nullptr);
        return;
    }

    if (entity->GetType() == EntityType::Projectile)
    {
        // Projectile must be harmless as soon as it hits map geometry
        entity->DestroyAndDisableProjectileDamage();
        collider->m_collisions.clear();
        collider->SetReferenceTile(nullptr);
        return;
    }

    std::sort(
        collider->m_collisions.begin(),
        collider->m_collisions.end(),
        [](const CollisionElement& lhs, const CollisionElement& rhs)
        {
            return lhs.m_area > rhs.m_area;
        });

    const float frameDt = SanitizeDeltaTime(deltaTime);

    for (const CollisionElement& hit : collider->m_collisions)
    {
        if (!hit.m_tile) continue;
        if (hit.m_tile->collision == TileCollision::None) continue;
        if (!IsValidRect(hit.m_tileBounds)) continue;

        const sf::FloatRect aabb = collider->GetAABB();
        if (!IsValidRect(aabb)) continue;

        std::optional<sf::FloatRect> intersection = aabb.findIntersection(hit.m_tileBounds);
        if (!intersection.has_value()) continue;

        sf::Vector2f delta;
        delta.x = (aabb.position.x + (aabb.size.x * 0.5f)) -
            (hit.m_tileBounds.position.x + (hit.m_tileBounds.size.x * 0.5f));
        delta.y = (aabb.position.y + (aabb.size.y * 0.5f)) -
            (hit.m_tileBounds.position.y + (hit.m_tileBounds.size.y * 0.5f));

        sf::Vector2f deltaDiff;
        deltaDiff.x = std::abs(delta.x) - (aabb.size.x * 0.5f + hit.m_tileBounds.size.x * 0.5f);
        deltaDiff.y = std::abs(delta.y) - (aabb.size.y * 0.5f + hit.m_tileBounds.size.y * 0.5f);

        bool forceVerticalResolution = false;

        // One-way platform rules:
        // 1) Only collide while descending.
        // 2) Ignore intersections from below.
        // 3) Ignore if we were already below top in previous frame
        if (hit.m_tile->collision == TileCollision::OneWay)
        {
            const float vy = transform->GetVelocity().y;
            if (!IsFinite(vy) || vy <= 0.0f) continue;
            if (delta.y > 0.0f) continue;

            const float previousBottom = (aabb.position.y + aabb.size.y) - (vy * frameDt);
            const float tileTop = hit.m_tileBounds.position.y;
            if (previousBottom > tileTop + kOneWayFootTolerance) continue;

            forceVerticalResolution = true;
        }

        const bool resolveOnX = !forceVerticalResolution && (deltaDiff.x > deltaDiff.y);

        if (resolveOnX)
        {
            float resolveX = 0.0f;
            if (delta.x > 0.0f)
                resolveX = (hit.m_tileBounds.position.x + hit.m_tileBounds.size.x) - aabb.position.x;
            else
                resolveX = -((aabb.position.x + aabb.size.x) - hit.m_tileBounds.position.x);

            if (!IsFinite(resolveX) || resolveX == 0.0f) continue;

            const sf::Vector2f velocity = transform->GetVelocity();
            if ((delta.x > 0.0f && velocity.x < 0.0f) ||
                (delta.x < 0.0f && velocity.x > 0.0f))
            {
                transform->SetVelocity(0.0f, velocity.y);
            }

            transform->AddPosition(resolveX, 0.0f);
            collider->SetCollidingX(true);
        }
        else
        {
            float resolveY = 0.0f;
            if (delta.y > 0.0f)
                resolveY = (hit.m_tileBounds.position.y + hit.m_tileBounds.size.y) - aabb.position.y;
            else
                resolveY = -((aabb.position.y + aabb.size.y) - hit.m_tileBounds.position.y);

            if (!IsFinite(resolveY) || resolveY == 0.0f) continue;

            const sf::Vector2f velocity = transform->GetVelocity();
            if ((delta.y > 0.0f && velocity.y < 0.0f) ||
                (delta.y < 0.0f && velocity.y > 0.0f))
            {
                transform->SetVelocity(velocity.x, 0.0f);
            }

            transform->AddPosition(0.0f, resolveY);

            const bool firstVerticalContact = !collider->IsCollidingY();
            if (firstVerticalContact)
            {
                if (delta.y < 0.0f) collider->SetReferenceTile(hit.m_tile);
                else collider->SetReferenceTile(nullptr);
            }

            collider->SetCollidingY(true);
        }

        // Keep collider aligned after each correction
        collider->SetAABB(BuildEntityAABB(*transform));
    }

    collider->m_collisions.clear();

    if (!collider->IsCollidingY())
        collider->SetReferenceTile(nullptr);
}