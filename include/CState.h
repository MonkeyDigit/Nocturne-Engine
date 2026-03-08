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
        if (damage <= 0) return;
        if (m_state == EntityState::Dying || m_state == EntityState::Hurt) return;

        m_hitPoints -= damage;

        if (m_hitPoints <= 0)
        {
            m_hitPoints = 0;
            SetState(EntityState::Dying);
        }
        else
            SetState(EntityState::Hurt);
    }

    void SetHitPoints(int hp)
    {
        // Ensure valid spawn/config values
        const int safeHp = (hp > 0) ? hp : 1;
        m_hitPoints = safeHp;
        m_maxHitPoints = safeHp;
    }

    void InstantKill()
    {
        m_hitPoints = 0;
        SetState(EntityState::Dying);
    }

    int GetHitPoints() const { return m_hitPoints; }
    int GetMaxHitPoints() const { return m_maxHitPoints; }

private:
    EntityState m_state;
    int m_hitPoints;
    int m_maxHitPoints;
};