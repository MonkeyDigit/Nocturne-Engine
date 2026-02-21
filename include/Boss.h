#pragma once
#include <SFML/Graphics.hpp>
#include "Map.h"

class Boss
{
public:
    Boss(float startX, float startY);

    // We pass the player's position so the boss knows where to charge
    void Update(sf::Time deltaTime, const Map& map, sf::Vector2f playerPos);
    void Draw(sf::RenderWindow& window);

    sf::FloatRect GetBounds() const;
    void TakeDamage(int damage, float knockbackDirectionX);
    bool IsDead() const;

private:
    // --- AI STATES ---
    enum class State { Idle, Charge };

    // --- PHYSICS & COMBAT CONSTANTS ---
    const float GRAVITY = 950.0f;
    const float FRICTION = 750.0f;
    const float CHARGE_SPEED = 175.0f; // Very fast horizontal attack
    const float KNOCKBACK_FORCE_X = 75.0f; // Bosses take less knockback
    const float KNOCKBACK_FORCE_Y = 50.0f;
    const float INVULNERABILITY_DURATION = 0.2f;

    // --- VARIABLES ---
    sf::Vector2f m_position;
    sf::Vector2f m_velocity;
    int m_hp;
    float m_invulnerabilityTimer;

    // AI Variables
    State m_currentState;
    float m_stateTimer;
    int m_facingDirection;

    sf::RectangleShape m_shape;

    void ResolveCollisionsX(const Map& map);
    void ResolveCollisionsY(const Map& map);
};