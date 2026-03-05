#include <cmath>
#include <cstdlib>
#include "AISystem.h"
#include "EntityManager.h"
#include "CAIPatrol.h"
#include "CController.h"
#include "CTransform.h"
#include "CBoxCollider.h"
#include "CState.h"

void AISystem::Update(EntityManager& entityManager, float deltaTime)
{
    EntityBase* player = entityManager.Find("Player");
    sf::Vector2f playerPos;
    bool isPlayerAlive = false;

    // Find where the player is and if they are a valid target
    if (player)
    {
        CTransform* pTrans = player->GetComponent<CTransform>();
        CState* pState = player->GetComponent<CState>();

        // Ensure the player is actually alive
        if (pTrans && pState && pState->GetState() != EntityState::Dying)
        {
            playerPos = pTrans->GetPosition();
            isPlayerAlive = true;
        }
    }

    // Update all enemies
    for (auto& entityPair : entityManager.GetEntities())
    {
        EntityBase* entity = entityPair.second.get();
        if (entity->GetType() != EntityType::Enemy) continue;

        CAIPatrol* ai = entity->GetComponent<CAIPatrol>();
        CController* controller = entity->GetComponent<CController>();
        CTransform* transform = entity->GetComponent<CTransform>();
        CState* state = entity->GetComponent<CState>();

        if (!ai || !controller || !transform || !state || state->GetState() == EntityState::Dying) continue;

        // Reset controller inputs every frame
        controller->m_moveLeft = false;
        controller->m_moveRight = false;

        bool isChasing = false;

        // ===============================================
        // CHASE LOGIC
        // ===============================================
        if (isPlayerAlive)
        {
            sf::Vector2f diff = playerPos - transform->GetPosition();
            float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);

            if (dist < 100.0f) // Chase range
            {
                isChasing = true;
                ai->m_hasDestination = false; // Interrupt any patrol

                // DEADZONE: Stop pressing buttons if too close
                if (std::abs(diff.x) > 10.0f)
                {
                    if (diff.x < 0.0f)
                        controller->m_moveLeft = true;
                    else
                        controller->m_moveRight = true;
                }
            }
        }

        // ===============================================
        // PATROL LOGIC
        // ===============================================
        if (!isChasing)
        {
            if (ai->m_hasDestination)
            {
                // Reached destination?
                if (std::abs(ai->m_destination.x - transform->GetPosition().x) < 16.0f)
                {
                    ai->m_hasDestination = false;
                    continue;
                }

                // Walk towards destination
                if (ai->m_destination.x - transform->GetPosition().x > 0.0f)
                    controller->m_moveRight = true;
                else
                    controller->m_moveLeft = true;

                // Hit a wall? Stop and rethink
                CBoxCollider* collider = entity->GetComponent<CBoxCollider>();
                if (collider && collider->IsCollidingX())
                {
                    ai->m_hasDestination = false;
                }

                continue; // Skip the idle timer if we are walking
            }

            // Idle wait timer
            ai->m_elapsed += deltaTime;
            if (ai->m_elapsed < 1.0f) continue;

            ai->m_elapsed -= 1.0f;

            // Pick a new random destination
            int newX = rand() % 64 + 1;
            if (rand() % 2) newX = -newX;

            ai->m_destination.x = transform->GetPosition().x + static_cast<float>(newX);
            if (ai->m_destination.x < 0.0f) ai->m_destination.x = 0.0f;

            ai->m_hasDestination = true;
        }
    }
}