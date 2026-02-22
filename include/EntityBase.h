#pragma once
#include <SFML/Graphics.hpp>
#include <string>

class EntityManager; // Forward declaration

enum class EntityState
{
    Idle = 0, Walking, Jumping, Attacking, Hurt, Dying
};

enum class EntityType
{
    Base = 0,
    Enemy,
    Player
};

class EntityBase
{
    // EntityManager needs access to m_id and m_name during creation
    friend class EntityManager;
public:

    EntityBase(EntityManager& entityManager);
    virtual ~EntityBase() = default;

    void SetPosition(float x, float y);
    void SetPosition(const sf::Vector2f& pos);
    void SetSize(float x, float y);
    void SetState(EntityState state);

    void AddVelocity(float x, float y);
    void Accelerate(float x, float y);
    void SetAcceleration(float x, float y);
    void SetVelocity(float x, float y);
    void ApplyFriction(float x, float y);

    virtual void Update(float deltaTime);
    virtual void Draw(sf::RenderWindow& window) = 0;
    // Method for what THIS entity does TO the collider entity
    virtual void OnEntityCollision(EntityBase* collider, bool attack) = 0;

    unsigned int GetId() const;
    std::string GetName() const;
    EntityType GetType() const;
    EntityState GetState() const;
    sf::Vector2f GetPosition() const;
    sf::Vector2f GetSize() const;

protected:
    void UpdateAABB();
    void ResolveMapCollisionsX(); // Map collisions unificate per tutti!
    void ResolveMapCollisionsY();

    std::string m_name;
    EntityType m_type;
    unsigned int m_id;

    sf::Vector2f m_position;
    sf::Vector2f m_positionOld;
    sf::Vector2f m_velocity;
    sf::Vector2f m_maxVelocity;
    sf::Vector2f m_acceleration;
    sf::Vector2f m_friction;

    sf::Vector2f m_size;
    sf::FloatRect m_AABB;
    EntityState m_state;

    bool m_isGrounded;
    bool m_collidingOnX;
    bool m_collidingOnY;

    EntityManager& m_entityManager;
};