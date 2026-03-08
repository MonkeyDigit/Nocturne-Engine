#include "CameraSystem.h"
#include "SharedContext.h"
#include "EntityManager.h"
#include "CTransform.h"
#include "Map.h"
#include "Window.h"

void CameraSystem::Update(EntityManager& entityManager, const Map& map)
{
    sf::View view = entityManager.GetContext().m_window.GetGameView();

    // Find the target entity to follow (currently hardcoded to "Player")
    EntityBase* target = entityManager.Find("Player");
    if (!target)
    {
        m_currentView = view;
        return;
    }

    CTransform* transform = target->GetComponent<CTransform>();
    if (!transform)
    {
        m_currentView = view;
        return;
    }

    // Center the camera on the target's center position
    sf::Vector2f targetPos = transform->GetPosition();
    sf::Vector2f targetSize = transform->GetSize();
    view.setCenter({ targetPos.x, targetPos.y + targetSize.y * 0.5f });

    // Camera Clamping logic (Prevent the camera from showing out-of-bounds areas)
    sf::Vector2f viewCenter = view.getCenter();
    sf::Vector2f viewSize = view.getSize();

    float mapWidth = static_cast<float>(map.GetMapSize().x * map.GetTileSize());
    float mapHeight = static_cast<float>(map.GetMapSize().y * map.GetTileSize());

    // Horizontal bounds
    if (viewCenter.x - viewSize.x * 0.5f < 0.0f) viewCenter.x = viewSize.x * 0.5f;
    else if (viewCenter.x + viewSize.x * 0.5f > mapWidth) viewCenter.x = mapWidth - viewSize.x * 0.5f;

    // Vertical bounds
    if (viewCenter.y + viewSize.y * 0.5f > mapHeight) viewCenter.y = mapHeight - viewSize.y * 0.5f;
    else if (viewCenter.y - viewSize.y * 0.5f < 0.0f) viewCenter.y = viewSize.y * 0.5f;

    // Apply clamped view
    view.setCenter(viewCenter);
    m_currentView = view;
}