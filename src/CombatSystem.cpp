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

    // Double loop to check collisions between all unique pairs of entities
    for (auto it1 = entities.begin(); it1 != entities.end(); ++it1)
    {
        EntityBase* e1 = it1->second.get();
        if (!e1) continue;

        CBoxCollider* col1 = e1->GetComponent<CBoxCollider>();
        CTransform* trans1 = e1->GetComponent<CTransform>();
        CState* state1 = e1->GetComponent<CState>();

        // Skip entities that lack physical presence or are already dying
        if (!col1 || !trans1 || !state1 || state1->GetState() == EntityState::Dying) continue;

        for (auto it2 = std::next(it1); it2 != entities.end(); ++it2)
        {
            EntityBase* e2 = it2->second.get();
            if (!e2) continue;

            CBoxCollider* col2 = e2->GetComponent<CBoxCollider>();
            CTransform* trans2 = e2->GetComponent<CTransform>();
            CState* state2 = e2->GetComponent<CState>();

            if (!col2) continue;

            // Flag to check if bodies are physically touching
            bool bodiesTouching = col1->GetAABB().findIntersection(col2->GetAABB()).has_value();

            // ==========================================
            // --- PROJECTILE LOGIC ---
            // (Projectiles only care if their body touches another body)
            // ==========================================
            const bool isE1Projectile = (e1->GetType() == EntityType::Projectile);
            const bool isE2Projectile = (e2->GetType() == EntityType::Projectile);

            if (bodiesTouching && (isE1Projectile || isE2Projectile))
            {
                EntityBase* projEntity = isE1Projectile ? e1 : e2;
                EntityBase* targetEntity = isE1Projectile ? e2 : e1;

                CProjectile* projComp = projEntity->GetComponent<CProjectile>();
                CState* targetState = targetEntity->GetComponent<CState>();

                if (projComp &&
                    targetState &&
                    targetState->GetState() != EntityState::Dying &&
                    targetEntity->GetType() != EntityType::Projectile &&
                    targetEntity->GetType() != projComp->GetShooterType())
                {
                    targetState->TakeDamage(projComp->GetDamage());
                    projEntity->Destroy();
                }

                continue; // no melee if a projectile exploded
            }

            if (!trans1 || !state1 || state1->GetState() == EntityState::Dying) continue;
            if (!trans2 || !state2 || state2->GetState() == EntityState::Dying) continue;

            // ==========================================
            // --- MELEE LOGIC (Enemy vs Player) ---
            // (Evaluated EVEN IF bodies are NOT touching, because weapons have reach!)
            // ==========================================
            EntityBase* enemy = nullptr;
            EntityBase* player = nullptr;

            // Identify the roles in the collision
            if (e1->GetType() == EntityType::Enemy && e2->GetType() == EntityType::Player)
            {
                enemy = e1;
                player = e2;
            }
            else if (e1->GetType() == EntityType::Player && e2->GetType() == EntityType::Enemy)
            {
                enemy = e2;
                player = e1;
            }

            if (enemy && player)
            {
                CState* enemyState = enemy->GetComponent<CState>();
                CState* playerState = player->GetComponent<CState>();
                CTransform* enemyTrans = enemy->GetComponent<CTransform>();
                CTransform* playerTrans = player->GetComponent<CTransform>();
                CBoxCollider* enemyCol = enemy->GetComponent<CBoxCollider>();
                CBoxCollider* playerCol = player->GetComponent<CBoxCollider>();
                CSprite* enemySprite = enemy->GetComponent<CSprite>();

                // PLAYER HITS ENEMY (Sword vs Body)
                if (playerState->GetState() == EntityState::Attacking && enemyState->GetState() != EntityState::Hurt)
                {
                    sf::FloatRect playerSwordBox = GetWorldAttackAABB(player);

                    if (playerSwordBox.findIntersection(enemyCol->GetAABB()).has_value())
                    {
                        enemyState->TakeDamage(1);
                        enemyTrans->AddVelocity((playerTrans->GetPosition().x < enemyTrans->GetPosition().x) ? 200.0f : -200.0f, -100.0f);
                    }
                }

                // ENEMY HITS PLAYER (AttackBox vs Body)
                if (enemyState->GetState() == EntityState::Attacking && playerState->GetState() != EntityState::Hurt)
                {
                    sf::FloatRect enemyAttackBox = GetWorldAttackAABB(enemy);

                    if (enemyAttackBox.findIntersection(playerCol->GetAABB()).has_value())
                    {
                        playerState->TakeDamage(1);

                        if (enemyTrans->GetPosition().x > playerTrans->GetPosition().x)
                        {
                            playerTrans->AddVelocity(-playerTrans->GetSpeed().x, 0.0f);
                            if (enemySprite) enemySprite->SetDirection(Direction::Left);
                        }
                        else
                        {
                            playerTrans->AddVelocity(playerTrans->GetSpeed().x, 0.0f);
                            if (enemySprite) enemySprite->SetDirection(Direction::Right);
                        }
                    }
                }

                // ENEMY DECIDES TO ATTACK (Requires physical body touching)
                else if (enemyState->GetState() != EntityState::Attacking && enemyState->GetState() != EntityState::Hurt)
                {
                    if (bodiesTouching)
                    {
                        enemyState->SetState(EntityState::Attacking);
                    }
                }
            }
        }
    }
}