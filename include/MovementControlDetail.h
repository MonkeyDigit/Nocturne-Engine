#pragma once

#include <vector>
#include <SFML/System/Vector2.hpp>

struct GameplayTuning;
class EntityBase;
class EntityManager;

namespace MovementControlDetail
{
    struct ProjectileSpawnRequest
    {
        unsigned int shooterId = 0;
        sf::Vector2f position = { 0.0f, 0.0f };
        sf::Vector2f velocity = { 0.0f, 0.0f };
        int damage = 0;
        float lifetime = 0.0f;
    };

    void UpdateSingleControllableEntity(
        EntityBase& entity,
        const GameplayTuning& tuning,
        float deltaTime,
        std::vector<ProjectileSpawnRequest>& pendingProjectiles);

    void SpawnPendingProjectiles(
        EntityManager& entityManager,
        const std::vector<ProjectileSpawnRequest>& pendingProjectiles);
}