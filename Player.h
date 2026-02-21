#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Window/Keyboard.hpp>
#include "Map.h"

class Player
{
public:
    Player();

    void Update(sf::Time deltaTime, const Map& map);
    void Draw(sf::RenderWindow& window);

private:
    const float MAX_SPEED = 240.0f;       // px/s
    const float ACCELERATION = 3000.0f;   // px/s^2
    const float FRICTION = 2600.0f;       // px/s

    // Jump & Gravity constants
    const float GRAVITY = 1900.0f;        // px/s^2
    const float JUMP_FORCE = 650.0f;      // px/s
    const float JUMP_CUT_MULTIPLIER = 0.45f;
    const float COYOTE_TIME = 0.12f;      // seconds
    const float JUMP_BUFFER_TIME = 0.12f; // seconds

    sf::Vector2f m_position;
    sf::Vector2f m_velocity;

    // State & Timers
    bool m_isGrounded;
    bool m_wasJumpPressed; // To detect the exact frame the key is pressed/released
    float m_coyoteTimer;
    float m_jumpBufferTimer;

    // TODO: A simple rectangle for Phase 1 testing
    sf::RectangleShape m_shape;

    void ResolveCollisionsX(const Map& map);
    void ResolveCollisionsY(const Map& map);
};