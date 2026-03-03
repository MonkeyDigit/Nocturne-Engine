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
    void Load(const std::string& path);

    virtual void OnEntityCollision(EntityBase& collider, bool attack) = 0;
    virtual void Update(float deltaTime) override;

protected:
    void UpdateAttackAABB();

    float m_jumpVelocity;
    float m_coyoteTimer = 0.0f;     // Extra time to jump after falling off a ledge
    float m_jumpBufferTimer = 0.0f; // Remembers the jump input for a few frames before hitting the ground

    sf::FloatRect m_attackAABB;
    sf::Vector2f m_attackAABBoffset;

    // ECS
    CSprite* m_sprite;
};