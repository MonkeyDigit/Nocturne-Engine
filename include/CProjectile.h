#pragma once
#include "Component.h"
#include "EntityBase.h"

class CProjectile : public Component
{
public:
    CProjectile(EntityBase* owner)
        : Component(owner), m_damage(10), m_lifespan(2.0f), m_shooterType(EntityType::Base)
    {}

    void Awake() override {}

    void Update(float deltaTime) override
    {
        m_lifespan -= deltaTime;
        if (m_lifespan <= 0.0f)
        {
            m_owner->Destroy();
        }
    }

    // --- GETTERS & SETTERS ---
    void SetShooterType(EntityType type) { m_shooterType = type; }
    EntityType GetShooterType() const { return m_shooterType; }

    void SetDamage(int dmg) { m_damage = dmg; }
    int GetDamage() const { return m_damage; }

    void SetLifespan(float seconds)
    {
        // Clamp to a small positive value to avoid invalid lifetime
        m_lifespan = (seconds > 0.0f) ? seconds : 0.01f;
    }


private:
    int m_damage;
    float m_lifespan;
    EntityType m_shooterType;
};