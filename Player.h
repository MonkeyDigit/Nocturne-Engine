#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Window/Keyboard.hpp>

class Player
{
public:
    Player();

    void Update(sf::Time deltaTime);
    void Draw(sf::RenderWindow& window);

private:
    const float MAX_SPEED = 240.0f;       // px/s
    const float ACCELERATION = 3000.0f;   // px/s^2
    const float FRICTION = 2600.0f;       // px/s

    sf::Vector2f m_position;
    sf::Vector2f m_velocity;

    // TODO: A simple rectangle for Phase 1 testing
    sf::RectangleShape m_shape;
};