#pragma once

#include <SFML/Graphics.hpp>
#include "Component.h"

// This component defines an object's location in space, its size and how it moves
class CTransform : public Component
{
public:
    explicit CTransform(EntityBase* owner);

    void SetPosition(float x, float y);
    void SetPosition(const sf::Vector2f& pos);
    void AddPosition(float x, float y);
    const sf::Vector2f& GetPosition() const;

    void SetVelocity(float x, float y);
    void AddVelocity(float x, float y);
    const sf::Vector2f& GetVelocity() const;

    void SetMaxVelocity(float x, float y);
    const sf::Vector2f& GetMaxVelocity() const;

    void SetAcceleration(float x, float y);
    void AddAcceleration(float x, float y);
    const sf::Vector2f& GetAcceleration() const;

    void SetSpeed(float x, float y);
    const sf::Vector2f& GetSpeed() const;

    void SetFriction(float x, float y);
    const sf::Vector2f& GetFriction() const;

    void SetSize(float x, float y);
    const sf::Vector2f& GetSize() const;

private:
    // --- SPATIAL DATA ---
    sf::Vector2f m_position;
    sf::Vector2f m_size;

    // --- PHYSICS DATA ---
    sf::Vector2f m_velocity;
    sf::Vector2f m_maxVelocity;
    sf::Vector2f m_speed;
    sf::Vector2f m_acceleration;
    sf::Vector2f m_friction;
};