#include <cmath>
#include <cstdlib>
#include "Enemy.h"
#include "EntityManager.h"
#include "CState.h"
#include "CController.h"
#include "CAIPatrol.h"

Enemy::Enemy(EntityManager& entityManager)
    : Character(entityManager)
{
    m_type = EntityType::Enemy;
    AddComponent<CController>();
    AddComponent<CAIPatrol>();
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