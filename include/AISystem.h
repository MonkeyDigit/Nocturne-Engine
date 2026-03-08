#pragma once
#include <random>

class EntityManager;

class AISystem
{
public:
    AISystem();
    ~AISystem() = default;

    void Update(EntityManager& entityManager, float deltaTime);
private:
    std::mt19937 m_rng;
    std::uniform_int_distribution<int> m_patrolDistanceDist;
    std::bernoulli_distribution m_patrolDirectionDist;
};