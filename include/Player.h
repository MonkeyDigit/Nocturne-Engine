#pragma once
#include "Character.h"
#include "EventManager.h"

class Player : public Character
{
public:
    Player(EntityManager& entityManager);

    void OnEntityCollision(EntityBase& collider, bool attack) override;
};