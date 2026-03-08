#pragma once
#include <random>
#include <cstdint>

class EntityManager;

class AISystem
{
public:
    AISystem();
    ~AISystem() = default;

    void Update(EntityManager& entityManager, float deltaTime);

    // Allows deterministic AI behavior for debugging/replays.
    void SetSeed(std::uint32_t seed);
    std::uint32_t GetSeed() const;

private:
    std::mt19937 m_rng;
    std::uint32_t m_seed = 0;
};