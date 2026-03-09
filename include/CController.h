#pragma once
#include <string>
#include "Component.h"

class CController : public Component
{
public:
    CController(EntityBase* owner)
        : Component(owner),
        m_jumpVelocity(125.0f),
        m_jumpCancelMultiplier(0.5f),
        m_verticalAirThreshold(0.01f),
        m_horizontalWalkThreshold(0.2f),
        m_coyoteTimeWindow(0.10f),
        m_jumpBufferWindow(0.15f),
        m_attackCooldown(0.0f),
        m_rangedCooldown(0.20f),
        m_rangedSpeed(300.0f),
        m_rangedLifetime(1.5f),
        m_rangedDamage(10),
        m_coyoteTimer(0.0f),
        m_jumpBufferTimer(0.0f),
        m_attackCooldownTimer(0.0f),
        m_rangedCooldownTimer(0.0f),
        m_moveLeft(false),
        m_moveRight(false),
        m_jump(false),
        m_cancelJump(false),
        m_attack(false),
        m_rangedEnabled(false),   // Feature flag: ranged is opt-in per character
        m_attackRanged(false),
        m_rangedSpawnOffsetX(20.0f),
        m_rangedSpawnOffsetY(-22.0f),
        m_rangedSizeX(16.0f),
        m_rangedSizeY(16.0f),
        m_rangedSheetPath("media/spritesheets/Player.sheet"),
        m_rangedAnimation("Idle")
    {
    }

    // --- PLATFORMER TUNABLES ---
    float m_jumpVelocity;
    float m_jumpCancelMultiplier;
    float m_verticalAirThreshold;
    float m_horizontalWalkThreshold;
    float m_coyoteTimeWindow;
    float m_jumpBufferWindow;
    float m_attackCooldown;

    // --- RANGED ATTACK TUNABLES ---
    float m_rangedCooldown;
    float m_rangedSpeed;
    float m_rangedLifetime;
    int m_rangedDamage;
    float m_rangedSpawnOffsetX; // Forward offset (magnitude, direction applied automatically)
    float m_rangedSpawnOffsetY; // Vertical offset from pivot (negative = up)
    float m_rangedSizeX;
    float m_rangedSizeY;
    std::string m_rangedSheetPath;
    std::string m_rangedAnimation;

    // --- RUNTIME TIMERS ---
    float m_coyoteTimer;
    float m_jumpBufferTimer;
    float m_attackCooldownTimer;
    float m_rangedCooldownTimer;

    // --- ABILITY FLAGS ---
    bool m_rangedEnabled;

    // --- INPUT FLAGS ---
    bool m_moveLeft;
    bool m_moveRight;
    bool m_jump;
    bool m_cancelJump;
    bool m_attack;
    bool m_attackRanged;
};