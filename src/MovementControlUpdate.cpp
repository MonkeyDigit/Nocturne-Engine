#include "MovementControlSystem.h"
#include "EntityManager.h"
#include "SharedContext.h"
#include "MovementControlDetail.h"

void MovementControlSystem::Update(float deltaTime)
{
    if (!m_entityManager) return;

    std::vector<MovementControlDetail::ProjectileSpawnRequest> pendingProjectiles;
    pendingProjectiles.reserve(m_entityManager->GetEntities().size());

    const GameplayTuning& tuning = m_entityManager->GetContext().m_gameplayTuning;

    for (auto& entityPair : m_entityManager->GetEntities())
    {
        if (!entityPair.second) continue;

        MovementControlDetail::UpdateSingleControllableEntity(
            *entityPair.second,
            tuning,
            deltaTime,
            pendingProjectiles);
    }

    MovementControlDetail::SpawnPendingProjectiles(*m_entityManager, pendingProjectiles);
}