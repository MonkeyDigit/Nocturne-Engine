#pragma once
#include "EntityBase.h"
#include "Direction.h"
#include "CSprite.h"

class Character : public EntityBase
{
    friend class EntityManager;
public:
    Character(EntityManager& entityManager);
    virtual ~Character();

    void Move(Direction dir);
    void Jump();
    void CancelJump();
    void Attack();
    void TakeDamage(int damage);
    void Load(const std::string& path);

    virtual void OnEntityCollision(EntityBase& collider, bool attack) = 0;
    virtual void Update(float deltaTime) override;
    void Draw(sf::RenderWindow& window) override;

    int GetHitPoints() const;
    int GetMaxHitPoints() const;

protected:
    void UpdateAttackAABB();
    virtual void Animate();

    float m_jumpVelocity;
    float m_coyoteTimer = 0.0f;     // Extra time to jump after falling off a ledge
    float m_jumpBufferTimer = 0.0f; // Remembers the jump input for a few frames before hitting the ground
    int m_hitPoints;
    int m_maxHitPoints;

    sf::FloatRect m_attackAABB;
    sf::Vector2f m_attackAABBoffset;

    // ECS
    CSprite* m_sprite;
};