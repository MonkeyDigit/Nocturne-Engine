#pragma once
#include <limits>
#include "Component.h"

enum class EntityState
{
    Idle, Walking, Jumping, Attacking, Hurt, Dying
};

class CState : public Component
{
public:
    CState(EntityBase* owner)
        : Component(owner),
        m_state(EntityState::Idle),
        m_hitPoints(5),
        m_maxHitPoints(5),
        m_attackDamage(1),
        m_touchDamage(0),
        m_invulnerabilityTime(0.0f),
        m_invulnerabilityTimer(0.0f),
        m_attackInstance(0),
        m_lastDamageSourceId(std::numeric_limits<unsigned int>::max()),
        m_lastDamageSourceAttackInstance(0),
        m_attackKnockbackX(0.0f),
        m_attackKnockbackY(0.0f),
        m_hasAttackKnockbackOverride(false)
    {}

    void Update(float deltaTime) override
    {
        if (m_invulnerabilityTimer > 0.0f)
        {
            m_invulnerabilityTimer -= deltaTime;
            if (m_invulnerabilityTimer < 0.0f)
                m_invulnerabilityTimer = 0.0f;
        }
    }

    void SetState(EntityState state)
    {
        if (m_state == EntityState::Dying) return;

        // Count a new attack only when entering Attacking from a different state.
        if (state == EntityState::Attacking && m_state != EntityState::Attacking)
            ++m_attackInstance;

        m_state = state;
    }

    EntityState GetState() const { return m_state; }

    bool TakeDamage(int damage)
    {
        if (damage <= 0) return false;
        if (m_state == EntityState::Dying) return false;
        if (m_invulnerabilityTimer > 0.0f) return false;

        m_hitPoints -= damage;
        // Reset timer
        m_invulnerabilityTimer = m_invulnerabilityTime;

        if (m_hitPoints <= 0)
        {
            m_hitPoints = 0;
            SetState(EntityState::Dying);
        }
        else
            SetState(EntityState::Hurt);

        return true;
    }

    void SetHitPoints(int hp)
    {
        const int safeHp = (hp > 0) ? hp : 1;
        m_hitPoints = safeHp;
        m_maxHitPoints = safeHp;
    }

    void SetAttackDamage(int damage)
    {
        if (damage > 0) m_attackDamage = damage;
    }

    int GetAttackDamage() const { return m_attackDamage; }

    void SetTouchDamage(int damage)
    {
        m_touchDamage = (damage >= 0) ? damage : 0;
    }

    int GetTouchDamage() const { return m_touchDamage; }

    void SetInvulnerabilityTime(float seconds)
    {
        m_invulnerabilityTime = (seconds >= 0.0f) ? seconds : 0.0f;
    }

    float GetInvulnerabilityTime() const { return m_invulnerabilityTime; }
    float GetInvulnerabilityTimer() const { return m_invulnerabilityTimer; }

    void InstantKill()
    {
        m_hitPoints = 0;
        m_invulnerabilityTimer = 0.0f;
        SetState(EntityState::Dying);
    }

    int GetHitPoints() const { return m_hitPoints; }
    int GetMaxHitPoints() const { return m_maxHitPoints; }
    unsigned int GetAttackInstance() const { return m_attackInstance; }

    bool TakeDamageFromAttack(unsigned int attackerId, unsigned int attackerAttackInstance, int damage)
    {
        // Prevent multiple hits from the same attack instance.
        if (attackerId == m_lastDamageSourceId &&
            attackerAttackInstance == m_lastDamageSourceAttackInstance)
        {
            return false;
        }

        if (!TakeDamage(damage))
            return false;

        m_lastDamageSourceId = attackerId;
        m_lastDamageSourceAttackInstance = attackerAttackInstance;
        return true;
    }

    void SetAttackKnockback(float x, float y)
    {
        // X knockback is a magnitude; Y can be negative to launch upward.
        m_attackKnockbackX = (x >= 0.0f) ? x : 0.0f;
        m_attackKnockbackY = y;
        m_hasAttackKnockbackOverride = true;
    }

    float GetAttackKnockbackX() const { return m_attackKnockbackX; }
    float GetAttackKnockbackY() const { return m_attackKnockbackY; }
    bool HasAttackKnockbackOverride() const { return m_hasAttackKnockbackOverride; }

private:
    EntityState m_state;
    int m_hitPoints;
    int m_maxHitPoints;

    int m_attackDamage;
    int m_touchDamage;
    float m_invulnerabilityTime;
    float m_invulnerabilityTimer;
    unsigned int m_attackInstance;
    unsigned int m_lastDamageSourceId;
    unsigned int m_lastDamageSourceAttackInstance;
    float m_attackKnockbackX;
    float m_attackKnockbackY;
    bool m_hasAttackKnockbackOverride;
};