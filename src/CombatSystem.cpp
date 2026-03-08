#include "CombatSystem.h"
#include "EntityManager.h"
#include "EntityBase.h"
#include "CBoxCollider.h"
#include "CTransform.h"
#include "CState.h"
#include "CSprite.h"
#include "CProjectile.h"

sf::FloatRect GetWorldAttackAABB(EntityBase* entity)
{
    CBoxCollider* collider = entity->GetComponent<CBoxCollider>();
    CSprite* sprite = entity->GetComponent<CSprite>();

    sf::FloatRect bodyAABB = collider->GetAABB();
    sf::FloatRect attackRect = collider->GetAttackAABB();
    sf::Vector2f offset = collider->GetAttackAABBOffset();

    float attackX = 0.0f;
    if (sprite->GetDirection() == Direction::Right)
        attackX = (bodyAABB.position.x + bodyAABB.size.x) + offset.x;
    else
        attackX = bodyAABB.position.x - offset.x - attackRect.size.x;

    float attackY = bodyAABB.position.y + offset.y;

    return sf::FloatRect({ attackX, attackY }, { attackRect.size.x, attackRect.size.y });
}

void CombatSystem::Update(EntityManager& entityManager)
{
    auto& entities = entityManager.GetEntities();

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

        if (entity->GetType() == EntityType::Player) player = entity;
        else if (entity->GetType() == EntityType::Enemy) enemies.push_back(entity);
    }

    ResolveProjectiles(projectiles, targets);
    ResolveEnemyVsPlayer(player, enemies);
}

void CombatSystem::ResolveProjectiles(
    const std::vector<EntityBase*>& projectiles,
    const std::vector<EntityBase*>& targets)
{
    for (EntityBase* projEntity : projectiles)
    {
        CProjectile* projComp = projEntity->GetComponent<CProjectile>();
        CBoxCollider* projCol = projEntity->GetComponent<CBoxCollider>();
        if (!projComp || !projCol) continue;

        for (EntityBase* target : targets)
        {
            if (target->GetType() == EntityType::Projectile) continue;
            if (target->GetType() == projComp->GetShooterType()) continue;

            CState* targetState = target->GetComponent<CState>();
            CBoxCollider* targetCol = target->GetComponent<CBoxCollider>();
            if (!targetState || !targetCol || targetState->GetState() == EntityState::Dying) continue;

            if (projCol->GetAABB().findIntersection(targetCol->GetAABB()).has_value())
            {
                targetState->TakeDamage(projComp->GetDamage());
                projEntity->Destroy();
                break;
            }
        }
    }
}

void CombatSystem::ResolveEnemyVsPlayer(
    EntityBase* player,
    const std::vector<EntityBase*>& enemies)
{
    if (!player) return;

    CState* playerState = player->GetComponent<CState>();
    CTransform* playerTrans = player->GetComponent<CTransform>();
    CBoxCollider* playerCol = player->GetComponent<CBoxCollider>();
    CSprite* playerSprite = player->GetComponent<CSprite>();

    if (!playerState || !playerTrans || !playerCol || !playerSprite) return;
    if (playerState->GetState() == EntityState::Dying) return;

    for (EntityBase* enemy : enemies)
    {
        CState* enemyState = enemy->GetComponent<CState>();
        CTransform* enemyTrans = enemy->GetComponent<CTransform>();
        CBoxCollider* enemyCol = enemy->GetComponent<CBoxCollider>();
        CSprite* enemySprite = enemy->GetComponent<CSprite>();

        if (!enemyState || !enemyTrans || !enemyCol || !enemySprite) continue;
        if (enemyState->GetState() == EntityState::Dying) continue;

        const bool bodiesTouching =
            enemyCol->GetAABB().findIntersection(playerCol->GetAABB()).has_value();

        if (playerState->GetState() == EntityState::Attacking &&
            enemyState->GetState() != EntityState::Hurt)
        {
            sf::FloatRect playerSwordBox = GetWorldAttackAABB(player);
            if (playerSwordBox.findIntersection(enemyCol->GetAABB()).has_value())
            {
                enemyState->TakeDamage(1);
                enemyTrans->AddVelocity(
                    (playerTrans->GetPosition().x < enemyTrans->GetPosition().x) ? 200.0f : -200.0f,
                    -100.0f);
            }
        }

        if (enemyState->GetState() == EntityState::Attacking &&
            playerState->GetState() != EntityState::Hurt)
        {
            sf::FloatRect enemyAttackBox = GetWorldAttackAABB(enemy);
            if (enemyAttackBox.findIntersection(playerCol->GetAABB()).has_value())
            {
                playerState->TakeDamage(1);

                if (enemyTrans->GetPosition().x > playerTrans->GetPosition().x)
                {
                    playerTrans->AddVelocity(-playerTrans->GetSpeed().x, 0.0f);
                    enemySprite->SetDirection(Direction::Left);
                }
                else
                {
                    playerTrans->AddVelocity(playerTrans->GetSpeed().x, 0.0f);
                    enemySprite->SetDirection(Direction::Right);
                }
            }
        }
        else if (enemyState->GetState() != EntityState::Attacking &&
            enemyState->GetState() != EntityState::Hurt)
        {
            if (bodiesTouching)
                enemyState->SetState(EntityState::Attacking);
        }
    }
}