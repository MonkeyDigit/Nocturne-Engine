#include <limits>

#include "CState.h"

CState::CState(EntityBase* owner)
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
{
}

void CState::Update(float deltaTime)
{
    if (m_invulnerabilityTimer > 0.0f)
    {
        m_invulnerabilityTimer -= deltaTime;
        if (m_invulnerabilityTimer < 0.0f)
            m_invulnerabilityTimer = 0.0f;
    }
}

void CState::SetState(EntityState state)
{
    if (m_state == EntityState::Dying) return;

    // Count a new attack only when entering Attacking from a different state
    if (state == EntityState::Attacking && m_state != EntityState::Attacking)
        ++m_attackInstance;

    m_state = state;
}

EntityState CState::GetState() const { return m_state; }

bool CState::TakeDamage(int damage)
{
    if (damage <= 0) return false;
    if (m_state == EntityState::Dying) return false;
    if (m_invulnerabilityTimer > 0.0f) return false;

    m_hitPoints -= damage;
    m_invulnerabilityTimer = m_invulnerabilityTime;

    if (m_hitPoints <= 0)
    {
        m_hitPoints = 0;
        SetState(EntityState::Dying);
    }
    else
    {
        SetState(EntityState::Hurt);
    }

    return true;
}

void CState::SetHitPoints(int hp)
{
    const int safeHp = (hp > 0) ? hp : 1;
    m_hitPoints = safeHp;
    m_maxHitPoints = safeHp;
}

void CState::SetAttackDamage(int damage)
{
    if (damage > 0) m_attackDamage = damage;
}

int CState::GetAttackDamage() const { return m_attackDamage; }

void CState::SetTouchDamage(int damage)
{
    m_touchDamage = (damage >= 0) ? damage : 0;
}

int CState::GetTouchDamage() const { return m_touchDamage; }

void CState::SetInvulnerabilityTime(float seconds)
{
    m_invulnerabilityTime = (seconds >= 0.0f) ? seconds : 0.0f;
}

float CState::GetInvulnerabilityTime() const { return m_invulnerabilityTime; }
float CState::GetInvulnerabilityTimer() const { return m_invulnerabilityTimer; }

void CState::InstantKill()
{
    m_hitPoints = 0;
    m_invulnerabilityTimer = 0.0f;
    SetState(EntityState::Dying);
}

int CState::GetHitPoints() const { return m_hitPoints; }
int CState::GetMaxHitPoints() const { return m_maxHitPoints; }
unsigned int CState::GetAttackInstance() const { return m_attackInstance; }

bool CState::TakeDamageFromAttack(unsigned int attackerId, unsigned int attackerAttackInstance, int damage)
{
    // Prevent multiple hits from the same attack instance
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

void CState::SetAttackKnockback(float x, float y)
{
    // X knockback is a magnitude; Y can be negative to launch upward
    m_attackKnockbackX = (x >= 0.0f) ? x : 0.0f;
    m_attackKnockbackY = y;
    m_hasAttackKnockbackOverride = true;
}

float CState::GetAttackKnockbackX() const { return m_attackKnockbackX; }
float CState::GetAttackKnockbackY() const { return m_attackKnockbackY; }
bool CState::HasAttackKnockbackOverride() const { return m_hasAttackKnockbackOverride; }