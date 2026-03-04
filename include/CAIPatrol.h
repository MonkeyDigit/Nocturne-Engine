#pragma once
#include "Component.h"
#include <SFML/System/Vector2.hpp>

class CAIPatrol : public Component
{
public:
    CAIPatrol(EntityBase* owner) : Component(owner), m_hasDestination(false), m_elapsed(0.0f) {}

    bool m_hasDestination;
    sf::Vector2f m_destination;
    float m_elapsed;
};