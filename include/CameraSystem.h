#pragma once
#include <SFML/Graphics.hpp>

class EntityManager;
class Map;

class CameraSystem
{
public:
    // Updates the camera position based on the target entity and map boundaries
    void Update(EntityManager& entityManager, const Map& map);
    const sf::View& GetCurrentView() const { return m_currentView; }
private:
    sf::View m_currentView;
};