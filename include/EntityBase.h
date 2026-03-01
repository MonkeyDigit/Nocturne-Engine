#pragma once
#include "Map.h"
#include <vector>

class EntityManager;

struct CollisionElement
{
    CollisionElement(float area, TileInfo* info, const sf::FloatRect& bounds)
        : m_area(area), m_tile(info), m_tileBounds(bounds) {
    }

    float m_area;
    TileInfo* m_tile;
    sf::FloatRect m_tileBounds;
};

using Collisions = std::vector<CollisionElement>;

bool SortCollisions(const CollisionElement& e1, const CollisionElement& e2);

enum class EntityState
{
    Idle, Walking, Jumping, Attacking, Hurt, Dying
};

// Moved here to solve circular dependency issues
enum class EntityType
{
    Base = 0, Enemy, Player
};

class EntityBase
{
    friend class EntityManager;
public:
    EntityBase(EntityManager& entityManager);
    virtual ~EntityBase();

    void SetPosition(float x, float y);
    void SetPosition(const sf::Vector2f& pos);
    void SetSize(float x, float y);
    void SetState(EntityState state);
    void Move(float x, float y);
    void AddVelocity(float x, float y);
    void Accelerate(float x, float y);
    void SetAcceleration(float x, float y);
    void SetVelocity(float x, float y);
    void ApplyFriction(float x, float y);

    virtual void Update(float deltaTime);
    virtual void Draw(sf::RenderWindow& window) = 0;
    // Method for what THIS entity does TO the collider entity
    virtual void OnEntityCollision(EntityBase& collider, bool attack) = 0;

    EntityType GetType() const;
    std::string GetName() const;
    unsigned int GetId() const;
    EntityState GetState() const;
    sf::Vector2f GetPosition() const;
    sf::Vector2f GetSize() const;

protected:
    void UpdateAABB();
    void CheckCollisions();
    void ResolveCollisions();

    std::string m_name;
    EntityType m_type;
    unsigned int m_id;
    sf::Vector2f m_position;
    sf::Vector2f m_positionOld;
    sf::Vector2f m_velocity;
    sf::Vector2f m_maxVelocity;
    sf::Vector2f m_speed;
    sf::Vector2f m_acceleration;
    sf::Vector2f m_friction;
    TileInfo* m_referenceTile;
    sf::Vector2f m_size;
    sf::FloatRect m_AABB;
    EntityState m_state;
    bool m_jumping;

    bool m_collidingOnX;
    bool m_collidingOnY;
    Collisions m_collisions;
    EntityManager& m_entityManager;
};