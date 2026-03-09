#pragma once
#include <string>

struct GameplayTuning
{
    // Combat fallback values
    float m_playerSwordKnockbackX = 200.0f;
    float m_playerSwordKnockbackY = -100.0f;
    float m_enemyAttackKnockbackX = 128.0f;
    float m_enemyAttackKnockbackY = 0.0f;

    // Projectile fallback values
    float m_projectileFallbackWidth = 16.0f;
    float m_projectileFallbackHeight = 16.0f;
    std::string m_projectileFallbackSheet = "media/spritesheets/Player.sheet";
    std::string m_projectileFallbackAnimation = "Idle";

    // Camera
    float m_cameraTargetVerticalBias = 0.5f;
};