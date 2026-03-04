#pragma once
#include "EventManager.h"

class EntityManager;

class MovementControlSystem
{
public:
    MovementControlSystem() = default;
    ~MovementControlSystem() = default;

    // Register callbacks (using a ptr instead of reference in order to store it in this class)
    void Initialize(EntityManager* entityManager);
    void Destroy();

    // Update the physics and timers based on input
    void Update(float deltaTime);
    // The callback triggered by SFML events
    void React(EventDetails& details);

private:
    EntityManager* m_entityManager = nullptr; // Keeps track of the manager
};