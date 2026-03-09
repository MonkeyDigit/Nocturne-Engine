#pragma once

#include "Component.h"

enum class EntityState
{
    Idle, Walking, Jumping, Attacking, Hurt, Dying
};

class CState : public Component
{
public:
    explicit CState(EntityBase* owner);

    void Update(float deltaTime) override;
    void SetState(EntityState state);
    EntityState GetState() const;

    bool TakeDamage(int damage);
    void SetHitPoints(int hp);

    void SetAttackDamage(int damage);
    int GetAttackDamage() const;

    void SetTouchDamage(int damage);
    int GetTouchDamage() const;

    void SetInvulnerabilityTime(float seconds);
    float GetInvulnerabilityTime() const;
    float GetInvulnerabilityTimer() const;

    void InstantKill();

    int GetHitPoints() const;
    int GetMaxHitPoints() const;
    unsigned int GetAttackInstance() const;

    bool TakeDamageFromAttack(unsigned int attackerId, unsigned int attackerAttackInstance, int damage);

    void SetAttackKnockback(float x, float y);
    float GetAttackKnockbackX() const;
    float GetAttackKnockbackY() const;
    bool HasAttackKnockbackOverride() const;

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