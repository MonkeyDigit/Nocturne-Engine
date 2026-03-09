#pragma once
#include <vector>

class EntityManager;
class EntityBase;
struct GameplayTuning;

class CombatSystem
{
public:
    CombatSystem() = default;
    ~CombatSystem() = default;

    // Checks collisions between entities and applies combat rules
    void Update(EntityManager& entityManager);

private:
    void ResolveProjectiles(
        const std::vector<EntityBase*>& projectiles,
        const std::vector<EntityBase*>& targets);

    void ResolveEnemyVsPlayer(
        EntityBase* player,
        const std::vector<EntityBase*>& enemies,
        const GameplayTuning& tuning);
};
