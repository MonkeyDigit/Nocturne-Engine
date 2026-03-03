#pragma once
#include "Character.h"
#include "EventManager.h"

class Player : public Character
{
public:
    Player(EntityManager& entityManager);
    ~Player() override;

    void OnEntityCollision(EntityBase& collider, bool attack) override;

    // The single callback that dispatches all actions
    void React(EventDetails& details);
    // TODO: RIMETTERE LE ADVANCED PLATFORMER MECHANICS O INTEGRARE NEI TIPI DI TILE?
};