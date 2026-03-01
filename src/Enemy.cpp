#include "Enemy.h"
#include "EntityManager.h"
#include <cmath>
#include <cstdlib>

Enemy::Enemy(EntityManager& entityManager)
    : Character(entityManager), m_hasDestination(false), m_elapsed(0.0f)
{
    m_type = EntityType::Enemy;
}

Enemy::~Enemy() {}

void Enemy::OnEntityCollision(EntityBase& collider, bool attack)
{
    if (m_state == EntityState::Dying) return;

    if (attack) return;

    if (collider.GetType() != EntityType::Player) return;

    Character& player = static_cast<Character&>(collider);
    SetState(EntityState::Attacking);
    player.TakeDamage(1);

    if (GetPosition().x > player.GetPosition().x)
    {
        player.AddVelocity(-GetSpeed().x, 0.0f);
        m_sprite->GetSpriteSheet().SetDirection(Direction::Left);
    }
    else
    {
        player.AddVelocity(GetSpeed().x, 0.0f);
        m_sprite->GetSpriteSheet().SetDirection(Direction::Right);
    }
}

void Enemy::Update(float deltaTime)
{
    Character::Update(deltaTime);

    if (m_hasDestination)
    {
        if (std::abs(m_destination.x - GetPosition().x) < 16.0f)
        {
            m_hasDestination = false;
            return;
        }

        if (m_destination.x - GetPosition().x > 0.0f) Move(Direction::Right);
        else Move(Direction::Left);

        if (m_collidingOnX) m_hasDestination = false;

        return;
    }

    m_elapsed += deltaTime;

    if (m_elapsed < 1.0f) return;

    m_elapsed -= 1.0f;

    int newX = rand() % 64 + 1;
    if (rand() % 2) newX = -newX;

    m_destination.x = GetPosition().x + static_cast<float>(newX);

    if (m_destination.x < 0.0f) m_destination.x = 0.0f;

    m_hasDestination = true;
}

void Enemy::Animate()
{
    // Simple basic animation fallback for enemies (extracted from your original Character class)
    EntityState state = GetState();
    Animation* currentAnimation = m_sprite->GetSpriteSheet().GetCurrentAnim();

    if (state == EntityState::Walking && currentAnimation->GetName() != "Walk")
    {
        m_sprite->GetSpriteSheet().SetAnimation("Walk", true, true);
    }
    else if (state == EntityState::Jumping && currentAnimation->GetName() != "Jump")
    {
        m_sprite->GetSpriteSheet().SetAnimation("Jump", true, false);
    }
    else if (state == EntityState::Attacking && currentAnimation->GetName() != "Attack")
    {
        m_sprite->GetSpriteSheet().SetAnimation("Attack", true, false);
    }
    else if (state == EntityState::Hurt && currentAnimation->GetName() != "Hurt")
    {
        m_sprite->GetSpriteSheet().SetAnimation("Hurt", true, false);
    }
    else if (state == EntityState::Dying && currentAnimation->GetName() != "Death")
    {
        m_sprite->GetSpriteSheet().SetAnimation("Death", true, false);
    }
    else if (state == EntityState::Idle && currentAnimation->GetName() != "Idle")
    {
        m_sprite->GetSpriteSheet().SetAnimation("Idle", true, true);
    }
}