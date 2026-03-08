#include <cmath>
#include "AISystem.h"
#include "EntityManager.h"
#include "CAIPatrol.h"
#include "CController.h"
#include "CTransform.h"
#include "CBoxCollider.h"
#include "CState.h"
#include "CSprite.h"

AISystem::AISystem()
    : m_rng(std::random_device{}())
{}

void AISystem::Update(EntityManager& entityManager, float deltaTime)
{
    EntityBase* player = entityManager.Find("Player");
    sf::Vector2f playerPos;
    bool isPlayerAlive = false;

    // Cache player position once per frame for all enemies
    if (player)
    {
        CTransform* pTrans = player->GetComponent<CTransform>();
        CState* pState = player->GetComponent<CState>();

        // Dead players are ignored as chase targets
        if (pTrans && pState && pState->GetState() != EntityState::Dying)
        {
            playerPos = pTrans->GetPosition();
            isPlayerAlive = true;
        }
    }

    for (auto& entityPair : entityManager.GetEntities())
    {
        EntityBase* entity = entityPair.second.get();
        if (entity->GetType() != EntityType::Enemy) continue;

        CAIPatrol* ai = entity->GetComponent<CAIPatrol>();
        CController* controller = entity->GetComponent<CController>();
        CTransform* transform = entity->GetComponent<CTransform>();
        CState* state = entity->GetComponent<CState>();
        CSprite* sprite = entity->GetComponent<CSprite>();

        if (!ai || !controller || !transform || !state || state->GetState() == EntityState::Dying) continue;

        // AI writes intent each frame; movement system consumes these flags later
        controller->m_moveLeft = false;
        controller->m_moveRight = false;

        // Clamp/fallback values to keep AI stable even with bad data
        const float chaseRange = (ai->m_chaseRange > 0.0f) ? ai->m_chaseRange : 100.0f;
        const float chaseDeadZone = (ai->m_chaseDeadZone >= 0.0f) ? ai->m_chaseDeadZone : 10.0f;
        const float arrivalThreshold = (ai->m_arrivalThreshold >= 0.0f) ? ai->m_arrivalThreshold : 16.0f;
        const float idleInterval = (ai->m_idleInterval > 0.0f) ? ai->m_idleInterval : 1.0f;

        bool isChasing = false;

        // Attack/chase decision is AI-driven
        // Priority: attack first, then chase, then patrol
        if (isPlayerAlive)
        {
            const sf::Vector2f diff = playerPos - transform->GetPosition();
            const float distSq = diff.x * diff.x + diff.y * diff.y;

            const float attackRangeX = (ai->m_attackRangeX > 0.0f) ? ai->m_attackRangeX : 26.0f;
            const float attackRangeY = (ai->m_attackRangeY > 0.0f) ? ai->m_attackRangeY : 20.0f;
            const bool inAttackWindow =
                std::abs(diff.x) <= attackRangeX &&
                std::abs(diff.y) <= attackRangeY;

            // Keep enemy facing toward player even in dead-zone.
            if (sprite && std::abs(diff.x) > 1.0f)
                sprite->SetDirection((diff.x < 0.0f) ? Direction::Left : Direction::Right);

            const float chaseRangeSq = chaseRange * chaseRange;

            const EntityState currentState = state->GetState();
            const bool canTriggerAttack =
                inAttackWindow &&
                controller->m_attackCooldownTimer <= 0.0f &&
                currentState != EntityState::Dying &&
                currentState != EntityState::Hurt &&
                currentState != EntityState::Attacking;

            if (canTriggerAttack)
            {
                // Stop movement this frame to make attack startup cleaner
                controller->m_moveLeft = false;
                controller->m_moveRight = false;
                controller->m_attack = true;
                ai->m_hasDestination = false;
                continue; // Attack has priority over patrol this frame
            }

            // If attack is not triggered, chase within chase range
            if (distSq < chaseRangeSq)
            {
                isChasing = true;
                ai->m_hasDestination = false; // Interrupt patrol while chasing

                // Dead-zone avoids jitter when very close to target
                if (std::abs(diff.x) > chaseDeadZone)
                {
                    if (diff.x < 0.0f) controller->m_moveLeft = true;
                    else controller->m_moveRight = true;
                }
            }
        }

        if (!isChasing)
        {
            if (ai->m_hasDestination)
            {
                // Stop when close enough to destination
                if (std::abs(ai->m_destination.x - transform->GetPosition().x) < arrivalThreshold)
                {
                    ai->m_hasDestination = false;
                    continue;
                }

                // Move toward destination
                if (ai->m_destination.x - transform->GetPosition().x > 0.0f) controller->m_moveRight = true;
                else controller->m_moveLeft = true;

                // Abort patrol step if blocked by wall
                CBoxCollider* collider = entity->GetComponent<CBoxCollider>();
                if (collider && collider->IsCollidingX()) ai->m_hasDestination = false;

                continue;
            }

            // Wait before choosing a new patrol destination
            ai->m_elapsed += deltaTime;
            if (ai->m_elapsed < idleInterval) continue;
            ai->m_elapsed -= idleInterval;

            int minDistance = (ai->m_patrolMinDistance > 0) ? ai->m_patrolMinDistance : 1;
            int maxDistance = (ai->m_patrolMaxDistance >= minDistance) ? ai->m_patrolMaxDistance : minDistance;

            float directionChance = ai->m_patrolDirectionChance;
            if (directionChance < 0.0f) directionChance = 0.0f;
            else if (directionChance > 1.0f) directionChance = 1.0f;

            std::uniform_int_distribution<int> patrolDistanceDist(minDistance, maxDistance);
            std::bernoulli_distribution patrolDirectionDist(directionChance);

            // Pick a random signed X offset
            int newX = patrolDistanceDist(m_rng);
            if (patrolDirectionDist(m_rng)) newX = -newX;

            ai->m_destination.x = transform->GetPosition().x + static_cast<float>(newX);
            if (ai->m_destination.x < 0.0f) ai->m_destination.x = 0.0f;

            ai->m_hasDestination = true;
        }
    }
}