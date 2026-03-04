#pragma once
class EntityManager;

class CombatSystem
{
public:
    CombatSystem() = default;
    ~CombatSystem() = default;

    // Checks collisions between entities and applies combat rules
    void Update(EntityManager& entityManager);
};