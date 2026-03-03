#include "RenderSystem.h"
#include "EntityManager.h"
#include "CSprite.h"
#include "CTransform.h"
#include "CBoxCollider.h"

void RenderSystem::Render(EntityManager& entityManager, sf::RenderWindow& window)
{
    sf::View view = window.getView();
    sf::FloatRect viewSpace(
        { view.getCenter().x - (view.getSize().x * 0.5f), view.getCenter().y - (view.getSize().y * 0.5f) },
        { view.getSize().x, view.getSize().y }
    );

    for (auto& entityPair : entityManager.GetEntities())
    {
        EntityBase* entity = entityPair.second.get();

        CSprite* sprite = entity->GetComponent<CSprite>();
        CTransform* transform = entity->GetComponent<CTransform>();

        if (sprite && transform)
        {
            // --- CULLING OPTIMIZATION ---
            CBoxCollider* collider = entity->GetComponent<CBoxCollider>();
            if (collider)
            {
                if (!collider->GetAABB().findIntersection(viewSpace).has_value())
                    continue;
            }

            sprite->GetSpriteSheet().SetSpritePosition(transform->GetPosition());
            sprite->Draw(window);
        }
    }
}