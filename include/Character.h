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

    void Load(const std::string& path);

    virtual void Update(float deltaTime) override;

protected:
    void UpdateAttackAABB();

    sf::FloatRect m_attackAABB;
    sf::Vector2f m_attackAABBoffset;

    // ECS
    CSprite* m_sprite;
};