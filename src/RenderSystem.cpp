#include <vector>
#include <algorithm>
#include "RenderSystem.h"
#include "CTransform.h"
#include "EntityManager.h"
#include "CSprite.h"
#include "CBoxCollider.h"

void RenderSystem::Render(EntityManager& entityManager, sf::RenderWindow& window)
{
    struct RenderItem
    {
        CSprite* sprite;
        float sortKeyY;
    };

    sf::View view = window.getView();
    sf::FloatRect viewSpace(
        { view.getCenter().x - (view.getSize().x * 0.5f), view.getCenter().y - (view.getSize().y * 0.5f) },
        { view.getSize().x, view.getSize().y }
    );

    std::vector<RenderItem> renderQueue;
    renderQueue.reserve(entityManager.GetEntities().size());

    for (auto& entityPair : entityManager.GetEntities())
    {
        EntityBase* entity = entityPair.second.get();
        if (!entity) continue;

        CSprite* sprite = entity->GetComponent<CSprite>();
        if (!sprite) continue;

        CBoxCollider* collider = entity->GetComponent<CBoxCollider>();
        if (collider && !collider->GetAABB().findIntersection(viewSpace).has_value())
            continue;

        float y = 0.0f;
        if (CTransform* transform = entity->GetComponent<CTransform>())
            y = transform->GetPosition().y;
        else if (collider)
            y = collider->GetAABB().position.y + collider->GetAABB().size.y;

        renderQueue.push_back({ sprite, y });
    }

    // Lower Y first, higher Y last (in front)
    std::stable_sort(renderQueue.begin(), renderQueue.end(),
        [](const RenderItem& a, const RenderItem& b)
        {
            return a.sortKeyY < b.sortKeyY;
        });

    for (const RenderItem& item : renderQueue)
        item.sprite->Draw(window);
}