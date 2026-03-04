#pragma once
class EntityManager;

class AISystem
{
public:
    AISystem() = default;
    ~AISystem() = default;

    void Update(EntityManager& entityManager, float deltaTime);
};