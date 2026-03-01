#include "Player.h"
#include "SharedContext.h"
#include "EntityManager.h"
#include "StateManager.h"

Player::Player(EntityManager& entityManager)
    : Character(entityManager)
{
    Load("media/characters/Player.char");
    m_type = EntityType::Player;

    EventManager& events = m_entityManager.GetContext().m_eventManager;

    // Registering the callbacks using the Nocturne Engine architecture
    // !!! IMPORTANT !!! REMEMBER TO REMOVE EACH CALLBACK
    events.AddCallback(StateType::Game, "Player_MoveLeft", &Player::React, *this);
    events.AddCallback(StateType::Game, "Player_MoveRight", &Player::React, *this);
    events.AddCallback(StateType::Game, "Player_Jump", &Player::React, *this);
    events.AddCallback(StateType::Game, "Player_Jump_Cancel", &Player::React, *this);
    events.AddCallback(StateType::Game, "Player_Attack", &Player::React, *this);
}

Player::~Player()
{
    EventManager& events = m_entityManager.GetContext().m_eventManager;
    events.RemoveCallback(StateType::Game, "Player_MoveLeft");
    events.RemoveCallback(StateType::Game, "Player_MoveRight");
    events.RemoveCallback(StateType::Game, "Player_Jump");
    events.RemoveCallback(StateType::Game, "Player_Jump_Cancel");
    events.RemoveCallback(StateType::Game, "Player_Attack");
}

void Player::OnEntityCollision(EntityBase& collider, bool attack)
{
    if (m_state == EntityState::Dying) return;

    if (attack)
    {
        if (m_state != EntityState::Attacking) return;
        if (!m_sprite->GetSpriteSheet().GetCurrentAnim()->IsPlaying()) return;

        if (collider.GetType() != EntityType::Enemy && collider.GetType() != EntityType::Player)
        {
            return;
        }

        Character& opponent = static_cast<Character&>(collider);
        opponent.TakeDamage(1);

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

void Player::React(EventDetails& details)
{
    std::string action = details.m_name;

    if (action == "Player_MoveLeft")            Character::Move(Direction::Left);
    else if (action == "Player_MoveRight")      Character::Move(Direction::Right);
    else if (action == "Player_Attack")         Character::Attack();
    else if (action == "Player_Jump")           Character::Jump();
    else if (action == "Player_Jump_Cancel")    Character::CancelJump();
}

void Player::Animate()
{
    EntityState state = GetState();
    Animation* currentAnimation = m_sprite->GetSpriteSheet().GetCurrentAnim();

    if (state == EntityState::Walking && currentAnimation->GetName() != "Walk")
    {
        if ((currentAnimation->GetName() == "WalkStart" && !currentAnimation->IsPlaying()) ||
            currentAnimation->GetName() == "Jump" ||
            currentAnimation->GetName() == "Fall")
        {
            m_sprite->GetSpriteSheet().SetAnimation("Walk", true, true);
        }
        else
        {
            m_sprite->GetSpriteSheet().SetAnimation("WalkStart", true, false);
        }
    }
    else if (state == EntityState::Jumping)
    {
        if (GetVelocity().y > 0.0f && currentAnimation->GetName() != "Fall")
        {
            m_sprite->GetSpriteSheet().SetAnimation("Fall", true, false);
        }
        else if (GetVelocity().x != 0.0f && GetVelocity().y < 0.0f && currentAnimation->GetName() != "JumpForward")
        {
            m_sprite->GetSpriteSheet().SetAnimation("JumpForward", true, false);
        }
        else if (GetVelocity().y < 0.0f && GetVelocity().x == 0.0f &&
            currentAnimation->GetName() != "Jump" &&
            currentAnimation->GetName() != "JumpForward")
        {
            m_sprite->GetSpriteSheet().SetAnimation("Jump", true, false);
        }
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
        if (currentAnimation->GetName() == "Walk")
        {
            m_sprite->GetSpriteSheet().SetAnimation("WalkEnd", true, false);
        }
        else if (currentAnimation->GetName() == "Fall")
        {
            m_sprite->GetSpriteSheet().SetAnimation("Land", true, false);
        }

        if ((currentAnimation->GetName() == "WalkEnd" && currentAnimation->IsPlaying()) ||
            (currentAnimation->GetName() == "Land" && currentAnimation->IsPlaying()))
        {
            return;
        }

        m_sprite->GetSpriteSheet().SetAnimation("Idle", true, true);
    }
}