#pragma once
#include "Component.h"
#include <SFML/System/Vector2.hpp>

class CAIPatrol : public Component
{
public:
    CAIPatrol(EntityBase* owner)
        : Component(owner),
        m_hasDestination(false),
        m_destination(0.0f, 0.0f),
        m_elapsed(0.0f),
        m_chaseRange(100.0f),
        m_chaseDeadZone(10.0f),
        m_arrivalThreshold(16.0f),
        m_idleInterval(1.0f),
        m_patrolMinDistance(1),
        m_patrolMaxDistance(64),
        m_patrolDirectionChance(0.5f),
        m_attackRangeX(26.0f),
        m_attackRangeY(20.0f)
    {}

    bool m_hasDestination;
    sf::Vector2f m_destination;
    float m_elapsed;

    // --- AI TUNABLES ---
    float m_chaseRange;
    float m_chaseDeadZone;
    float m_arrivalThreshold;
    float m_idleInterval;
    int m_patrolMinDistance;
    int m_patrolMaxDistance;
    float m_patrolDirectionChance;
    // Attack trigger window in local 2D space
    // X controls horizontal reach, Y controls vertical tolerance
    float m_attackRangeX;
    float m_attackRangeY;
};