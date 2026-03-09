#include "CombatSystem.h"
#include "EntityManager.h"
#include "EntityBase.h"
#include "CBoxCollider.h"
#include "CTransform.h"
#include "CState.h"
#include "CProjectile.h"
#include "SharedContext.h"
#include "GameplayTuning.h"
#include "EngineLog.h"

void CombatSystem::Update(EntityManager& entityManager)
{
    auto& entities = entityManager.GetEntities();
    const GameplayTuning& tuning = entityManager.GetContext().m_gameplayTuning;

    std::vector<EntityBase*> projectiles;
    std::vector<EntityBase*> targets;
    std::vector<EntityBase*> enemies;
    EntityBase* player = nullptr;

    for (auto& pair : entities)
    {
        EntityBase* entity = pair.second.get();
        if (!entity) continue;

        CBoxCollider* col = entity->GetComponent<CBoxCollider>();
        if (!col) continue;

        if (entity->GetType() == EntityType::Projectile)
        {
            if (entity->GetComponent<CProjectile>())
                projectiles.push_back(entity);
            continue;
        }

        CState* state = entity->GetComponent<CState>();
        CTransform* trans = entity->GetComponent<CTransform>();
        if (!state || !trans || state->GetState() == EntityState::Dying) continue;

        targets.push_back(entity);

        if (entity->GetType() == EntityType::Player)
        {
            if (!player)
            {
                player = entity;
            }
            else
            {
                EngineLog::WarnOnce(
                    "combat.player.multiple",
                    "Multiple Player entities found during combat update. Using the first one.");
            }
        }
        else if (entity->GetType() == EntityType::Enemy)
        {
            enemies.push_back(entity);
        }
    }

    ResolveProjectiles(projectiles, targets);
    ResolveEnemyVsPlayer(player, enemies, tuning);
}