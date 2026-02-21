#pragma once
#include <SFML/Graphics.hpp>
#include "Map.h"

class Enemy
{
public:
    Enemy(float startX, float startY);

    void Update(sf::Time deltaTime, const Map& map);
    void Draw(sf::RenderWindow& window);

    // Get the hitbox of the enemy
    sf::FloatRect GetBounds() const;

    // Called when the player hits the enemy
    void TakeDamage(int damage, float knockbackDirectionX);

private:
    // --- PHYSICS & COMBAT CONSTANTS ---
    const float GRAVITY = 950.0f;       // Same as player for consistency
    const float FRICTION = 750.0f;      // How fast it stops sliding after knockback
    const float KNOCKBACK_FORCE_X = 175.0f;
    const float KNOCKBACK_FORCE_Y = 125.0f; // Small upward pop when hit
    const float INVULNERABILITY_DURATION = 0.3f;

    // --- STATE ---
    sf::Vector2f m_position;
    sf::Vector2f m_velocity;
    int m_hp;
    float m_invulnerabilityTimer;
    int m_facingDirection;


    // --- VISUALS & ANIMATION ---
    sf::RectangleShape m_shape;
    sf::Texture m_texture;
    sf::Sprite m_sprite;

    enum class AnimState { Idle, Attack, Walk, Hurt };
    AnimState m_animState;
    float m_animTimer;
    int m_currentFrame;

    const int SPRITE_WIDTH = 32;
    const int SPRITE_HEIGHT = 32;

    // Collision resolution logic (similar to Player)
    void ResolveCollisionsX(const Map& map);
    void ResolveCollisionsY(const Map& map);
};