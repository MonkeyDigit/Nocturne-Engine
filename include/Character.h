#pragma once
#include "EntityBase.h"
#include "SpriteSheet.h"
#include "Direction.h"

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

    SpriteSheet m_spriteSheet;
    float m_jumpVelocity;
    int m_hitPoints;
    int m_maxHitPoints;

    sf::FloatRect m_attackAABB;
    sf::Vector2f m_attackAABBoffset;
};