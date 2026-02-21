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

    void UnlockDoubleJump();

    sf::Vector2f GetPosition() const;
    // Getters for combat
    sf::FloatRect GetAttackBounds() const;
    bool IsAttacking() const;

    int GetFacingDirection() const;
    sf::FloatRect GetBounds() const;

private:
    const float MAX_SPEED = 120.0f;       // px/s
    const float ACCELERATION = 1500.0f;   // px/s^2
    const float FRICTION = 1300.0f;       // px/s

    // --- JUMP & GRAVITY CONSTANTS ---
    const float GRAVITY = 950.0f;          // px/s^2
    const float JUMP_FORCE = 325.0f;        // px/s
    const float DOUBLE_JUMP_FORCE = 275.0f; // Slightly weaker than the main jump
    const float JUMP_CUT_MULTIPLIER = 0.45f;
    const float COYOTE_TIME = 0.12f;        // seconds
    const float JUMP_BUFFER_TIME = 0.12f;   // seconds

    // --- COMBAT CONSTANTS ---
    const float ATTACK_DURATION = 0.2f; // How long the attack hitbox stays active

    sf::Vector2f m_position;
    sf::Vector2f m_velocity;

    // --- STATE & TIMERS ---
    bool m_isGrounded;
    bool m_wasJumpPressed; // To detect the exact frame the key is pressed/released
    float m_coyoteTimer;
    float m_jumpBufferTimer;
    bool m_wasAttackPressed;

    // --- COMBAT STATES ---
    int m_facingDirection; // 1 for Right, -1 for Left
    bool m_isAttacking;
    float m_attackTimer;

    // --- ABILITY STATES ---
    bool m_hasDoubleJumpUnlocked; // Do we have the relic?
    bool m_canDoubleJump;         // Is it available right now in the air?

    // --- VISUALS & ANIMATION ---
    sf::Texture m_texture;
    sf::Sprite m_sprite;

    enum class AnimState { Idle, Run, Jump, Fall, Attack };
    AnimState m_animState;
    float m_animTimer;
    int m_currentFrame;

    // TODO: LA DIMENSIONE VIENE FORNITA TRAMITE IL FILE CFG
    const int SPRITE_WIDTH = 140;
    const int SPRITE_HEIGHT = 95;

    // TODO: A simple rectangle for Phase 1 testing
    sf::RectangleShape m_shape;
    sf::RectangleShape m_attackHitbox; // The visual "sword/whip"

    void ResolveCollisionsX(const Map& map);
    void ResolveCollisionsY(const Map& map);
};