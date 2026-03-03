#include <cmath>
#include <cstdlib>
#include "Enemy.h"
#include "EntityManager.h"
#include "CState.h"

Enemy::Enemy(EntityManager& entityManager)
    : Character(entityManager), m_hasDestination(false), m_elapsed(0.0f)
{
    m_type = EntityType::Enemy;
}

Enemy::~Enemy() {}

void Enemy::OnEntityCollision(EntityBase& collider, bool attack)
{
    if (this->GetComponent<CState>()->GetState() == EntityState::Dying) return;

    if (attack) return;

    if (collider.GetType() != EntityType::Player) return;

    Character& player = static_cast<Character&>(collider);
    this->GetComponent<CState>()->SetState(EntityState::Attacking);

    player.GetComponent<CState>()->TakeDamage(1);

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

        if (m_collider->IsCollidingX()) m_hasDestination = false;

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