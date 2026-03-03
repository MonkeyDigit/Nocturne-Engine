#pragma once
#include <SFML/Graphics.hpp>

// Forward declaration to avoid circular dependencies
class EntityManager;

class RenderSystem
{
public:
    RenderSystem() = default;
    ~RenderSystem() = default;

    // Automatically syncs positions and draws entities with a CSprite
    void Render(EntityManager& entityManager, sf::RenderWindow& window);
};