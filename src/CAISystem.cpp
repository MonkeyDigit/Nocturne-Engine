#include "AISystem.h"
#include "EntityManager.h"
#include "CAIPatrol.h"
#include "CController.h"
#include "CTransform.h"
#include "CBoxCollider.h"
#include <cmath>
#include <cstdlib>

void AISystem::Update(EntityManager& entityManager, float deltaTime)
{
    for (auto& entityPair : entityManager.GetEntities())
    {
        EntityBase* entity = entityPair.second.get();

        CAIPatrol* ai = entity->GetComponent<CAIPatrol>();
        CController* controller = entity->GetComponent<CController>();
        CTransform* transform = entity->GetComponent<CTransform>();

        if (!ai || !controller || !transform) continue;

        if (ai->m_hasDestination)
        {
            if (std::abs(ai->m_destination.x - transform->GetPosition().x) < 16.0f)
            {
                ai->m_hasDestination = false;
                continue;
            }

            if (ai->m_destination.x - transform->GetPosition().x > 0.0f)
                controller->m_moveRight = true;
            else
                controller->m_moveLeft = true;

            CBoxCollider* collider = entity->GetComponent<CBoxCollider>();
            if (collider && collider->IsCollidingX())
                ai->m_hasDestination = false;

            continue;
        }

        ai->m_elapsed += deltaTime;
        if (ai->m_elapsed < 1.0f) continue;

        ai->m_elapsed -= 1.0f;

        int newX = rand() % 64 + 1;
        if (rand() % 2) newX = -newX;

        ai->m_destination.x = transform->GetPosition().x + static_cast<float>(newX);
        if (ai->m_destination.x < 0.0f) ai->m_destination.x = 0.0f;

        ai->m_hasDestination = true;
    }
}