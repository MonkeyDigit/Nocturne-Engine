#pragma once
#include "EntityBase.h"
#include "SpriteSheet.h"

class Character : public EntityBase
{
    friend class EntityManager;
public:
    Character(EntityManager& entityManager);
    virtual ~Character() = default;

    // Load reads properties from .char files (Data-Driven approach)
    void Load(const std::string& path);

    virtual void OnEntityCollision(EntityBase* collider, bool attack) = 0;
    virtual void Update(float deltaTime) override;
    virtual void Draw(sf::RenderWindow& window) override;

    int GetHitPoints() const;
    int GetMaxHitPoints() const;
    void TakeDamage(int damage);

protected:
    void UpdateAttackAABB();
    virtual void Animate();

    SpriteSheet m_spriteSheet;

    float m_jumpVelocity;
    int m_hitPoints;
    int m_maxHitPoints;

    sf::FloatRect m_attackAABB;
    sf::Vector2f m_attackAABBoffset;
    float m_invulnerabilityTimer;
};