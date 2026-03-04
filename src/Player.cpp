#include "Player.h"
#include "EntityManager.h"
#include "CController.h"
#include "CState.h"

Player::Player(EntityManager& entityManager)
    : Character(entityManager)
{
    // IMPORTANT: Add Controller Component first, then load character data
    AddComponent<CController>();
    Load("media/characters/Player.char");
    m_type = EntityType::Player;

}

void Player::OnEntityCollision(EntityBase& collider, bool attack)
{
    if (this->GetComponent<CState>()->GetState() == EntityState::Dying) return;

    if (attack)
    {
        if (this->GetComponent<CState>()->GetState() != EntityState::Attacking) return;
        if (!m_sprite->GetSpriteSheet().GetCurrentAnim()->IsPlaying()) return;

        if (collider.GetType() != EntityType::Enemy && collider.GetType() != EntityType::Player)
        {
            return;
        }

        Character& opponent = static_cast<Character&>(collider);
        opponent.GetComponent<CState>()->TakeDamage(1);

        if (GetPosition().x > opponent.GetPosition().x)
        {
            opponent.AddVelocity(-32.0f, 0.0f);
        }
        else
        {
            opponent.AddVelocity(32.0f, 0.0f);
        }
    }
    else
    {
        // Other behavior
    }
}