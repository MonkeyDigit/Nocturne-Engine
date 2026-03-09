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
}

void PhysicsSystem::CheckMapCollisions(EntityBase* entity, Map* map)
{
    CTransform* transform = entity->GetComponent<CTransform>();
    CBoxCollider* collider = entity->GetComponent<CBoxCollider>();

    (void)transform; // kept for symmetry with existing access pattern

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

    if (entity->GetType() == EntityType::Projectile)
    {
        // Ensure projectile cannot deal damage after map collision in the same frame
        entity->DestroyAndDisableProjectileDamage();
        collider->m_collisions.clear();
        collider->SetReferenceTile(nullptr);
        return;
    }

    // Resolve the most significant overlaps first
    // This helps reduce jitter on tile seams and corners
    std::sort(collider->m_collisions.begin(), collider->m_collisions.end(),
        [](const CollisionElement& e1, const CollisionElement& e2) {
            return e1.m_area > e2.m_area;
        });

    const float tileSize = static_cast<float>(map->GetTileSize());

    for (auto& itr : collider->m_collisions)
    {
        const sf::FloatRect& aabb = collider->GetAABB();
        std::optional<sf::FloatRect> intersection = aabb.findIntersection(itr.m_tileBounds);
        if (!intersection.has_value()) continue;

        // Delta between entity center and tile center
        sf::Vector2f delta;
        delta.x = (aabb.position.x + (aabb.size.x * 0.5f)) - (itr.m_tileBounds.position.x + (itr.m_tileBounds.size.x * 0.5f));
        delta.y = (aabb.position.y + (aabb.size.y * 0.5f)) - (itr.m_tileBounds.position.y + (itr.m_tileBounds.size.y * 0.5f));

        // Overlap metrics used to choose the primary resolution axis
        sf::Vector2f deltaDiff;
        deltaDiff.x = std::abs(delta.x) - (aabb.size.x * 0.5f + itr.m_tileBounds.size.x * 0.5f);
        deltaDiff.y = std::abs(delta.y) - (aabb.size.y * 0.5f + itr.m_tileBounds.size.y * 0.5f);

        bool forceVerticalResolution = false;

        // One-way platform rules:
        // 1) Only collide while descending
        // 2) Ignore hits coming from below
        // 3) If we were already below platform top last frame, keep falling through
        if (itr.m_tile->collision == TileCollision::OneWay)
        {
            if (transform->GetVelocity().y <= 0.0f) continue;
            if (delta.y > 0.0f) continue;

            const float frameDt = (deltaTime > 0.0f) ? deltaTime : kFallbackFixedDt;
            const float moveY = transform->GetVelocity().y * frameDt;
            const float previousBottom = (aabb.position.y + aabb.size.y) - moveY;
            const float tileTop = itr.m_tileBounds.position.y;

            if (previousBottom > tileTop + kOneWayFootTolerance) continue;

            // Valid landing from above: always resolve on Y
            forceVerticalResolution = true;
        }

        double resolve = 0.0;
        const bool resolveOnX = !forceVerticalResolution && (deltaDiff.x > deltaDiff.y);

        if (resolveOnX)
        {
            // Horizontal separation
            if (delta.x > 0.0f) resolve = (itr.m_tileBounds.position.x + tileSize) - aabb.position.x;
            else resolve = -(((double)aabb.position.x + (double)aabb.size.x) - (double)itr.m_tileBounds.position.x);

            // Cancel X velocity only if moving into the blocking side
            if ((delta.x > 0.0f && transform->GetVelocity().x < 0.0f) ||
                (delta.x < 0.0f && transform->GetVelocity().x > 0.0f))
            {
                transform->SetVelocity(0.0f, transform->GetVelocity().y);
            }

            transform->AddPosition(static_cast<float>(resolve), 0.0f);
            collider->SetCollidingX(true);
        }
        else
        {
            // Vertical separation
            if (delta.y > 0.0f) resolve = (itr.m_tileBounds.position.y + tileSize) - aabb.position.y;
            else resolve = -((aabb.position.y + aabb.size.y) - itr.m_tileBounds.position.y);

            // Cancel Y velocity only if moving into the blocking side
            if ((delta.y > 0.0f && transform->GetVelocity().y < 0.0f) ||
                (delta.y < 0.0f && transform->GetVelocity().y > 0.0f))
            {
                transform->SetVelocity(transform->GetVelocity().x, 0.0f);
            }

            transform->AddPosition(0.0f, static_cast<float>(resolve));

            // Keep the first valid ground-contact tile for this frame
            if (collider->IsCollidingY()) continue;

            if (delta.y < 0.0f) collider->SetReferenceTile(itr.m_tile);
            else collider->SetReferenceTile(nullptr);

            collider->SetCollidingY(true);
        }

        // Keep collider aligned after each correction to avoid cascading errors
        collider->SetAABB(sf::FloatRect(
            { transform->GetPosition().x - (transform->GetSize().x * 0.5f), transform->GetPosition().y - transform->GetSize().y },
            { transform->GetSize().x, transform->GetSize().y }
        ));
    }

    collider->m_collisions.clear();

    // If no vertical collision survived, entity is not grounded this frame
    if (!collider->IsCollidingY()) collider->SetReferenceTile(nullptr);
}