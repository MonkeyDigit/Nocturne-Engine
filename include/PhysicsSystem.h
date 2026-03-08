#pragma once
#include "Map.h"

// Forward declarations
class EntityBase;
class EntityManager;

class PhysicsSystem
{
public:
    PhysicsSystem() = default;
    ~PhysicsSystem() = default;

    // The main entry point called every frame
    void Update(EntityManager& entityManager, Map* gameMap, float deltaTime);

private:
    // Helper functions to break down physics logic
    void ApplyGravityAndMovement(EntityBase* entity, Map* map, float deltaTime);
    void CheckMapCollisions(EntityBase* entity, Map* map);
    void ResolveMapCollisions(EntityBase* entity, Map* map, float deltaTime);
    void ConstrainToMapBounds(EntityBase* entity, Map* map);
};