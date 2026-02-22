#include "Player.h"
#include "EntityManager.h"
#include "SharedContext.h"
#include "StateManager.h"
#include <iostream>

Player::Player(EntityManager& entityManager)
    : Character(entityManager),
    m_coyoteTimer(0.0f),
    m_jumpBufferTimer(0.0f)
{
    Load("media/characters/player.char");

    // --- REGISTERING CALLBACKS ---
    EventManager& evMgr = m_entityManager.GetContext().m_eventManager;

    // TODO: Assicurati che questi nomi (es. "MoveLeft") coincidano con quelli in Bindings.cfg
    evMgr.AddCallback(StateType::Game, "MoveLeft", &Player::MoveLeft, *this);
    evMgr.AddCallback(StateType::Game, "MoveRight", &Player::MoveRight, *this);
    evMgr.AddCallback(StateType::Game, "Jump", &Player::Jump, *this);
    evMgr.AddCallback(StateType::Game, "Attack", &Player::Attack, *this);
}

Player::~Player()
{
    // --- UNREGISTERING CALLBACKS ---
    EventManager& evMgr = m_entityManager.GetContext().m_eventManager;
    evMgr.RemoveCallback(StateType::Game, "MoveLeft");
    evMgr.RemoveCallback(StateType::Game, "MoveRight");
    evMgr.RemoveCallback(StateType::Game, "Jump");
    evMgr.RemoveCallback(StateType::Game, "Attack");
}

void Player::OnEntityCollision(EntityBase* collider, bool attack)
{
    if (collider->GetType() == EntityType::Enemy && !attack)
    {
        TakeDamage(1);

        if (m_position.x < collider->GetPosition().x) m_velocity.x = -m_maxVelocity.x * 0.5f;
        else m_velocity.x = m_maxVelocity.x * 0.5f;

        m_velocity.y = -m_jumpVelocity * 0.5f;
    }
}

void Player::Update(float deltaTime)
{
    // --- TIMERS ---
    m_coyoteTimer -= deltaTime;
    m_jumpBufferTimer -= deltaTime;

    if (m_isGrounded) m_coyoteTimer = COYOTE_TIME;

    if (m_state == EntityState::Attacking)
    {
        Animation* currentAnim = m_spriteSheet.GetCurrentAnim();
        if (currentAnim && !currentAnim->IsPlaying())
        {
            m_state = EntityState::Idle;
        }
    }

    // --- APPLY FORCES based on current state ---
    // Friction is applied automatically by EntityBase::Update if acceleration is 0

    // Set Logical State for Character::Animate() to use
    if (m_state != EntityState::Attacking && m_state != EntityState::Hurt)
    {
        if (!m_isGrounded) m_state = EntityState::Jumping;
        else if (std::abs(m_velocity.x) > 10.0f) m_state = EntityState::Walking;
        else m_state = EntityState::Idle;
    }

    // Move, Collide, Animate
    Character::Update(deltaTime);

    // Reset acceleration every frame so we stop moving if buttons are released!
    SetAcceleration(0.0f, m_acceleration.y);
}

// ==========================================
// EVENT MANAGER CALLBACKS
// ==========================================

void Player::MoveLeft(EventDetails& details)
{
    if (m_state == EntityState::Attacking) return; // Can't move while attacking

    m_spriteSheet.SetDirection(Direction::Left);
    Accelerate(-m_acceleration.x, 0.0f);
}

void Player::MoveRight(EventDetails& details)
{
    if (m_state == EntityState::Attacking) return;

    m_spriteSheet.SetDirection(Direction::Right);
    Accelerate(m_acceleration.x, 0.0f);
}

void Player::Jump(EventDetails& details)
{
    if (m_state == EntityState::Attacking) return;

    if (!details.m_heldDown) // JUST PRESSED
    {
        m_jumpBufferTimer = JUMP_BUFFER_TIME;

        // Execute Jump if buffered and within coyote time
        if (m_jumpBufferTimer > 0.0f && m_coyoteTimer > 0.0f)
        {
            m_velocity.y = -m_jumpVelocity;
            m_jumpBufferTimer = 0.0f;
            m_coyoteTimer = 0.0f;
        }
    }
    // TODO: CALLBACK FOR JUMP RELEASE
}

void Player::Attack(EventDetails& details)
{
    if (!details.m_heldDown && m_state != EntityState::Attacking)
    {
        m_state = EntityState::Attacking;
        m_velocity.x = 0.0f;
    }
}