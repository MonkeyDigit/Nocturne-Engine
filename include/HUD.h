#pragma once
#include <SFML/Graphics.hpp>

class EntityManager;

class HUD
{
public:
    HUD(EntityManager& entityManager);

    // Updates the health bar based on the player's current health
    void Update();

    // Draws the UI to the screen
    void Draw(sf::RenderWindow& window);

private:
    EntityManager& m_entityManager;

    sf::RectangleShape m_healthBarBackground;
    sf::RectangleShape m_healthBar;

    int m_maxHealth;
};