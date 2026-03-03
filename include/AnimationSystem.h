#pragma once
class EntityManager;

class AnimationSystem
{
public:
    AnimationSystem() = default;
    ~AnimationSystem() = default;

    void Update(EntityManager& entityManager, float deltaTime);
};