#pragma once
#include "Character.h"

class Enemy : public Character
{
public:
    Enemy(EntityManager& entityManager);
    ~Enemy() override;

    void OnEntityCollision(EntityBase& collider, bool attack) override;
    void Update(float deltaTime) override;

private:
    sf::Vector2f m_destination;
    bool m_hasDestination;
    float m_elapsed;
};