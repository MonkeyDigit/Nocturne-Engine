#include "CombatSystem.h"
#include "EntityManager.h"
#include "CBoxCollider.h"
#include "CTransform.h"
#include "CState.h"
#include "CSprite.h"

void CombatSystem::Update(EntityManager& entityManager)
{
    auto& entities = entityManager.GetEntities();

    // Double loop to check collisions between all unique pairs of entities
    for (auto it1 = entities.begin(); it1 != entities.end(); ++it1)
    {
        EntityBase* e1 = it1->second.get();
        CBoxCollider* col1 = e1->GetComponent<CBoxCollider>();
        CTransform* trans1 = e1->GetComponent<CTransform>();
        CState* state1 = e1->GetComponent<CState>();

        // Skip entities that lack physical presence or are already dying
        if (!col1 || !trans1 || !state1 || state1->GetState() == EntityState::Dying) continue;

        for (auto it2 = std::next(it1); it2 != entities.end(); ++it2)
        {
            EntityBase* e2 = it2->second.get();
            CBoxCollider* col2 = e2->GetComponent<CBoxCollider>();
            CTransform* trans2 = e2->GetComponent<CTransform>();
            CState* state2 = e2->GetComponent<CState>();

            if (!col2 || !trans2 || !state2 || state2->GetState() == EntityState::Dying) continue;

            // If the bounding boxes intersect, a physical collision has occurred!
            if (col1->GetAABB().findIntersection(col2->GetAABB()).has_value())
            {
                EntityBase* enemy = nullptr;
                EntityBase* player = nullptr;

                // Identify the roles in the collision
                if (e1->GetType() == EntityType::Enemy && e2->GetType() == EntityType::Player)
                {
                    enemy = e1; player = e2;
                }
                else if (e1->GetType() == EntityType::Player && e2->GetType() == EntityType::Enemy)
                {
                    enemy = e2; player = e1;
                }

                // If the collision involves an Enemy and a Player
                if (enemy && player)
                {
                    CState* enemyState = enemy->GetComponent<CState>();
                    CState* playerState = player->GetComponent<CState>();
                    CTransform* enemyTrans = enemy->GetComponent<CTransform>();
                    CTransform* playerTrans = player->GetComponent<CTransform>();
                    CSprite* enemySprite = enemy->GetComponent<CSprite>();

                    // Combat Logic: Apply damage if neither is currently attacking/hurt
                    if (enemyState->GetState() != EntityState::Attacking && playerState->GetState() != EntityState::Hurt)
                    {
                        enemyState->SetState(EntityState::Attacking);
                        playerState->TakeDamage(1);

                        // Knockback Calculation: Push the player away based on relative X positions
                        if (enemyTrans->GetPosition().x > playerTrans->GetPosition().x)
                        {
                            playerTrans->AddVelocity(-playerTrans->GetSpeed().x, 0.0f);
                            if (enemySprite) enemySprite->GetSpriteSheet().SetDirection(Direction::Left);
                        }
                        else
                        {
                            playerTrans->AddVelocity(playerTrans->GetSpeed().x, 0.0f);
                            if (enemySprite) enemySprite->GetSpriteSheet().SetDirection(Direction::Right);
                        }
                    }
                }
            }
        }
    }
}