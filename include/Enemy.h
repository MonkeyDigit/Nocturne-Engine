#pragma once
#include "Character.h"

class Enemy : public Character
{
public:
    Enemy(EntityManager& entityManager);
    ~Enemy() override;

    // TODO: Cosa fare di questo?
    void OnEntityCollision(EntityBase& collider, bool attack) override;
};