#pragma once

#include <SFML/Graphics.hpp>
#include "Component.h"

// This component defines an object's location in space, its size and how it moves
class CTransform : public Component
{
public:
    explicit CTransform(EntityBase* owner);

    void SetPosition(float x, float y) { m_position = sf::Vector2f(x, y); }
    void SetPosition(const sf::Vector2f& pos) { m_position = pos; }
    void AddPosition(float x, float y) { m_position.x += x; m_position.y += y; }
    const sf::Vector2f& GetPosition() const { return m_position; }

    void SetVelocity(float x, float y) { m_velocity = sf::Vector2f(x, y); }
    void AddVelocity(float x, float y); // non-trivial: keep in .cpp
    const sf::Vector2f& GetVelocity() const { return m_velocity; }

    void SetMaxVelocity(float x, float y) { m_maxVelocity = sf::Vector2f(x, y); }
    const sf::Vector2f& GetMaxVelocity() const { return m_maxVelocity; }

    void SetAcceleration(float x, float y) { m_acceleration = sf::Vector2f(x, y); }
    void AddAcceleration(float x, float y) { m_acceleration.x += x; m_acceleration.y += y; }
    const sf::Vector2f& GetAcceleration() const { return m_acceleration; }

    void SetSpeed(float x, float y) { m_speed = sf::Vector2f(x, y); }
    const sf::Vector2f& GetSpeed() const { return m_speed; }

    void SetFriction(float x, float y) { m_friction = sf::Vector2f(x, y); }
    const sf::Vector2f& GetFriction() const { return m_friction; }

    void SetSize(float x, float y) { m_size = sf::Vector2f(x, y); }
    const sf::Vector2f& GetSize() const { return m_size; }

private:
    sf::Vector2f m_position;
    sf::Vector2f m_size;

    sf::Vector2f m_velocity;
    sf::Vector2f m_maxVelocity;
    sf::Vector2f m_speed;
    sf::Vector2f m_acceleration;
    sf::Vector2f m_friction;
};