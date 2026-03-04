#pragma once
#pragma once
#include "Component.h"

class CController : public Component
{
public:
    CController(EntityBase* owner) : Component(owner),
        m_jumpVelocity(125.0f), m_coyoteTimer(0.0f), m_jumpBufferTimer(0.0f),
        m_moveLeft(false), m_moveRight(false), m_jump(false), m_cancelJump(false), m_attack(false) {
    }

    // --- PLATFORMER STATS ---
    float m_jumpVelocity;
    float m_coyoteTimer;
    float m_jumpBufferTimer;

    // --- INPUT INTENTIONS (Flags) ---
    bool m_moveLeft;
    bool m_moveRight;
    bool m_jump;
    bool m_cancelJump;
    bool m_attack;
};