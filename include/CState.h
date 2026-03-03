#pragma once
#include "Component.h"

enum class EntityState
{
    Idle, Walking, Jumping, Attacking, Hurt, Dying
};

class CState : public Component
{
public:
    CState(EntityBase* owner) : Component(owner), m_state(EntityState::Idle), m_hitPoints(5), m_maxHitPoints(5) {}

    void SetState(EntityState state)
    {
        if (m_state == EntityState::Dying) return;
        m_state = state;
    }

    EntityState GetState() const { return m_state; }

    void TakeDamage(int damage)
    {
        if (m_state == EntityState::Dying || m_state == EntityState::Hurt) return;

        m_hitPoints = (m_hitPoints - damage > 0) ? m_hitPoints - damage : 0;

        if (m_hitPoints > 0) SetState(EntityState::Hurt);
        else SetState(EntityState::Dying);
    }

    int GetHitPoints() const { return m_hitPoints; }
    int GetMaxHitPoints() const { return m_maxHitPoints; }
    void SetHitPoints(int hp) { m_hitPoints = hp; m_maxHitPoints = hp; }

private:
    EntityState m_state;
    int m_hitPoints;
    int m_maxHitPoints;
};