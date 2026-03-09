#include <cmath>

#include "CTransform.h"

namespace
{
    inline void ClampVelocityComponent(float& velocity, float maxVelocity)
    {
        // Clamp only when max velocity is configured (> 0).
        if (maxVelocity <= 0.0f) return;
        if (std::abs(velocity) <= maxVelocity) return;
        velocity = (velocity < 0.0f) ? -maxVelocity : maxVelocity;
    }
}

CTransform::CTransform(EntityBase* owner)
    : Component(owner),
    m_position(0.0f, 0.0f),
    m_size(0.0f, 0.0f),
    m_velocity(0.0f, 0.0f),
    m_maxVelocity(0.0f, 0.0f),
    m_speed(0.0f, 0.0f),
    m_acceleration(0.0f, 0.0f),
    m_friction(0.0f, 0.0f)
{
}

void CTransform::AddVelocity(float x, float y)
{
    m_velocity.x += x;
    m_velocity.y += y;

    ClampVelocityComponent(m_velocity.x, m_maxVelocity.x);
    ClampVelocityComponent(m_velocity.y, m_maxVelocity.y);
}