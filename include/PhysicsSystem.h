#pragma once

class Map;
class EntityBase;
class EntityManager;

class PhysicsSystem
{
public:
    PhysicsSystem() = default;
    ~PhysicsSystem() = default;

    void Update(EntityManager& entityManager, Map* gameMap, float deltaTime);

private:
    void ApplyGravityAndMovement(EntityBase* entity, Map* map, float deltaTime);
    void CheckMapCollisions(EntityBase* entity, Map* map);
    void ResolveMapCollisions(EntityBase* entity, Map* map, float deltaTime);
    bool ConstrainToMapBounds(EntityBase* entity, Map* map);
};