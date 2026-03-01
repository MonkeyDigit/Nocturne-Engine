#pragma once
#include "Component.h"
#include <SFML/Graphics.hpp>

// Forward declaration needed for TileInfo
struct TileInfo;

struct CollisionElement
{
    CollisionElement(float area, TileInfo* info, const sf::FloatRect& bounds)
        : m_area(area), m_tile(info), m_tileBounds(bounds) {
    }

    float m_area;
    TileInfo* m_tile;
    sf::FloatRect m_tileBounds;
};

class CBoxCollider : public Component
{
public:
    CBoxCollider(EntityBase* owner)
        : Component(owner), m_collidingOnX(false), m_collidingOnY(false), m_referenceTile(nullptr) {
    }

    // --- GETTERS & SETTERS ---

    void SetAABB(const sf::FloatRect& rect) { m_AABB = rect; }
    const sf::FloatRect& GetAABB() const { return m_AABB; }

    void SetCollidingX(bool col) { m_collidingOnX = col; }
    bool IsCollidingX() const { return m_collidingOnX; }

    void SetCollidingY(bool col) { m_collidingOnY = col; }
    bool IsCollidingY() const { return m_collidingOnY; }

    void SetReferenceTile(TileInfo* tile) { m_referenceTile = tile; }
    TileInfo* GetReferenceTile() const { return m_referenceTile; }

    // Public list of collisions for the current frame
    std::vector<CollisionElement> m_collisions;

private:
    sf::FloatRect m_AABB;       // The bounding box of the entity
    bool m_collidingOnX;        // Is it touching a wall?
    bool m_collidingOnY;        // Is it touching the floor or ceiling?
    TileInfo* m_referenceTile;  // The tile it's currently standing on
};