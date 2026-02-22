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
    events.AddCallback(StateType::Game, "Player_MoveLeft", &Player::React, *this);
    events.AddCallback(StateType::Game, "Player_MoveRight", &Player::React, *this);
    events.AddCallback(StateType::Game, "Player_Jump", &Player::React, *this);
    events.AddCallback(StateType::Game, "Player_Attack", &Player::React, *this);
}

Player::~Player()
{
    EventManager& events = m_entityManager.GetContext().m_eventManager;
    events.RemoveCallback(StateType::Game, "Player_MoveLeft");
    events.RemoveCallback(StateType::Game, "Player_MoveRight");
    events.RemoveCallback(StateType::Game, "Player_Jump");
    events.RemoveCallback(StateType::Game, "Player_Attack");
}

void Player::OnEntityCollision(EntityBase& collider, bool attack)
{
    if (m_state == EntityState::Dying) return;

    if (attack)
    {
        if (m_state != EntityState::Attacking) return;
        if (!m_spriteSheet.GetCurrentAnim()->IsPlaying()) return;

        if (collider.GetType() != EntityType::Enemy && collider.GetType() != EntityType::Player)
        {
            return;
        }

        Character& opponent = static_cast<Character&>(collider);
        opponent.TakeDamage(1);

        if (m_position.x > opponent.GetPosition().x)
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

    if (action == "Player_MoveLeft")
    {
        Character::Move(Direction::Left);
    }
    else if (action == "Player_MoveRight")
    {
        Character::Move(Direction::Right);
    }
    else if (action == "Player_Attack" && !details.m_heldDown)
    {
        Character::Attack();
    }
    else // For actions that happen once a key press
    {
        if (action == "Player_Jump" && !details.m_heldDown)
        {
            Character::Jump();
        }
    }
}

void Player::Animate()
{
    EntityState state = GetState();
    Animation* currentAnimation = m_spriteSheet.GetCurrentAnim();

    if (state == EntityState::Walking && currentAnimation->GetName() != "Walk")
    {
        if ((currentAnimation->GetName() == "WalkStart" && !currentAnimation->IsPlaying()) ||
            currentAnimation->GetName() == "Jump" ||
            currentAnimation->GetName() == "Fall")
        {
            m_spriteSheet.SetAnimation("Walk", true, true);
        }
        else
        {
            m_spriteSheet.SetAnimation("WalkStart", true, false);
        }
    }
    else if (state == EntityState::Jumping)
    {
        if (m_velocity.y > 0.0f && currentAnimation->GetName() != "Fall")
        {
            m_spriteSheet.SetAnimation("Fall", true, false);
        }
        else if (m_velocity.x != 0.0f && m_velocity.y < 0.0f && currentAnimation->GetName() != "JumpForward")
        {
            m_spriteSheet.SetAnimation("JumpForward", true, false);
        }
        else if (m_velocity.y < 0.0f && m_velocity.x == 0.0f &&
            currentAnimation->GetName() != "Jump" &&
            currentAnimation->GetName() != "JumpForward")
        {
            m_spriteSheet.SetAnimation("Jump", true, false);
        }
    }
    else if (state == EntityState::Attacking && currentAnimation->GetName() != "Attack")
    {
        m_spriteSheet.SetAnimation("Attack", true, false);
    }
    else if (state == EntityState::Hurt && currentAnimation->GetName() != "Hurt")
    {
        m_spriteSheet.SetAnimation("Hurt", true, false);
    }
    else if (state == EntityState::Dying && currentAnimation->GetName() != "Death")
    {
        m_spriteSheet.SetAnimation("Death", true, false);
    }
    else if (state == EntityState::Idle && currentAnimation->GetName() != "Idle")
    {
        if (currentAnimation->GetName() == "Walk")
        {
            m_spriteSheet.SetAnimation("WalkEnd", true, false);
        }
        else if (currentAnimation->GetName() == "Fall")
        {
            m_spriteSheet.SetAnimation("Land", true, false);
        }

        if ((currentAnimation->GetName() == "WalkEnd" && currentAnimation->IsPlaying()) ||
            (currentAnimation->GetName() == "Land" && currentAnimation->IsPlaying()))
        {
            return;
        }

        m_spriteSheet.SetAnimation("Idle", true, true);
    }
}