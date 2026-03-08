#include <algorithm>
#include "CameraSystem.h"
#include "SharedContext.h"
#include "EntityManager.h"
#include "CTransform.h"
#include "Map.h"
#include "Window.h"

namespace
{
    constexpr float kCameraTargetVerticalBias = 0.5f;

    float ClampAxisToMap(float center, float viewSize, float mapSize)
    {
        if (mapSize <= 0.0f) return center;

        // If the map is smaller than the view, keep the camera centered on the map
        if (mapSize <= viewSize) return mapSize * 0.5f;

        const float halfView = viewSize * 0.5f;
        return std::clamp(center, halfView, mapSize - halfView);
    }
}

void CameraSystem::Update(EntityManager& entityManager, const Map& map)
{
    sf::View view = entityManager.GetContext().m_window.GetGameView();

    EntityBase* target = entityManager.GetPlayer();
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

    const float mapWidth = static_cast<float>(map.GetMapSize().x * map.GetTileSize());
    const float mapHeight = static_cast<float>(map.GetMapSize().y * map.GetTileSize());
    if (mapWidth <= 0.0f || mapHeight <= 0.0f)
    {
        m_currentView = view;
        return;
    }

    const sf::Vector2f targetPos = transform->GetPosition();
    const sf::Vector2f targetSize = transform->GetSize();
    view.setCenter({ targetPos.x, targetPos.y + targetSize.y * kCameraTargetVerticalBias });

    sf::Vector2f viewCenter = view.getCenter();
    const sf::Vector2f viewSize = view.getSize();

    viewCenter.x = ClampAxisToMap(viewCenter.x, viewSize.x, mapWidth);
    viewCenter.y = ClampAxisToMap(viewCenter.y, viewSize.y, mapHeight);

    view.setCenter(viewCenter);
    m_currentView = view;
}