#include "Enemy.h"
#include "EntityManager.h"
#include <cmath>
#include <cstdlib>

Enemy::Enemy(EntityManager& entityManager)
    : Character(entityManager),
    m_hasDestination(false),
    m_waitTimer(0.0f)
{
    m_type = EntityType::Enemy;
    // NOTE: Statistics (Health, Speed, Spritesheet) are injected automatically by EntityManager calling the Load() method when the enemy is spawned
}

void Enemy::OnEntityCollision(EntityBase* collider, bool attack)
{
    if (m_state == EntityState::Dying) return;

    if (attack) return; // Handled by EntityManager's attack hitbox checks

    // If we bump into the player, get aggressive
    if (collider->GetType() == EntityType::Player)
    {
        SetState(EntityState::Attacking);

        // Face the player
        if (m_position.x > collider->GetPosition().x)
        {
            m_spriteSheet.SetDirection(Direction::Left);
        }
        else
        {
            m_spriteSheet.SetDirection(Direction::Right);
        }
    }
}

void Enemy::Update(float deltaTime)
{
    if (m_state == EntityState::Dying)
    {
        Character::Update(deltaTime);
        return;
    }

    // --- SIMPLE PATROL AI LOGIC ---
    if (m_hasDestination)
    {
        // Arrived at destination?
        if (std::abs(m_destination.x - m_position.x) < 16.0f || m_collidingOnX)
        {
            m_hasDestination = false;
        }
        else
        {
            // Move towards destination
            float moveDirection = (m_destination.x > m_position.x) ? 1.0f : -1.0f;
            Accelerate(moveDirection * m_acceleration.x, 0.0f);

            // Update facing direction
            m_spriteSheet.SetDirection(moveDirection > 0.0f ? Direction::Right : Direction::Left);
        }
    }
    else
    {
        // Waiting state
        m_waitTimer += deltaTime;
        if (m_waitTimer >= 1.0f)
        {
            m_waitTimer = 0.0f;
            m_hasDestination = true;

            // Pick a random distance to walk (from 16 to 80 pixels)
            int randomOffset = (std::rand() % 64) + 16;
            if (std::rand() % 2 == 0) randomOffset = -randomOffset;

            m_destination.x = m_position.x + static_cast<float>(randomOffset);

            // Prevent walking completely off the left edge of the map
            if (m_destination.x < 0.0f) m_destination.x = 0.0f;
        }
    }

    // Apply friction automatically if not actively steering
    if (m_acceleration.x == 0.0f) SetAcceleration(0.0f, m_acceleration.y);

    // --- ANIMATION STATE UPDATE ---
    if (m_state != EntityState::Attacking && m_state != EntityState::Hurt)
    {
        if (!m_isGrounded) m_state = EntityState::Jumping;
        else if (std::abs(m_velocity.x) > 10.0f) m_state = EntityState::Walking;
        else m_state = EntityState::Idle;
    }

    // Character::Update handles physics, collisions, and sprite updates
    Character::Update(deltaTime);

    // Reset acceleration for the next frame
    SetAcceleration(0.0f, m_acceleration.y);
}