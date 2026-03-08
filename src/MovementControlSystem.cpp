#include <cmath>
#include "MovementControlSystem.h"
#include "EntityManager.h"
#include "SharedContext.h"
#include "CController.h"
#include "CTransform.h"
#include "CState.h"
#include "CSprite.h"
#include "CBoxCollider.h"
#include "StateManager.h"

namespace
{
    inline void DecreaseToZero(float& value, float deltaTime)
    {
        if (value <= 0.0f) return;
        value -= deltaTime;
        if (value < 0.0f) value = 0.0f;
    }

    inline bool CanMove(EntityState state)
    {
        return state != EntityState::Dying &&
            state != EntityState::Attacking &&
            state != EntityState::Hurt;
    }

    inline bool CanQueueJump(EntityState state)
    {
        return state != EntityState::Dying &&
            state != EntityState::Hurt &&
            state != EntityState::Jumping &&
            state != EntityState::Attacking;
    }

    inline bool CanStartMeleeAttack(EntityState state, float cooldownTimer)
    {
        return cooldownTimer <= 0.0f &&
            state != EntityState::Dying &&
            state != EntityState::Hurt &&
            state != EntityState::Jumping &&
            state != EntityState::Attacking;
    }

    inline bool CanStartRangedAttack(EntityState state, float cooldownTimer)
    {
        return cooldownTimer <= 0.0f &&
            state != EntityState::Dying &&
            state != EntityState::Hurt &&
            state != EntityState::Attacking;
    }

    inline bool CanAutoState(EntityState state)
    {
        return state != EntityState::Dying &&
            state != EntityState::Attacking &&
            state != EntityState::Hurt;
    }

    inline EntityState ResolveLocomotionState(
        const sf::Vector2f& velocity,
        float verticalAirThreshold,
        float horizontalWalkThreshold)
    {
        if (std::abs(velocity.y) > verticalAirThreshold) return EntityState::Jumping;
        if (std::abs(velocity.x) > horizontalWalkThreshold) return EntityState::Walking;
        return EntityState::Idle;
    }

    inline bool IsGrounded(const CBoxCollider* collider)
    {
        return collider && collider->GetReferenceTile() != nullptr;
    }

    inline void SpawnRangedProjectile(
        EntityManager& entityManager,
        EntityBase& shooter,
        const CTransform& transform,
        const CSprite& sprite,
        const CController& controller)
    {
        sf::Vector2f velocity(controller.m_rangedSpeed, 0.0f);
        if (sprite.GetDirection() == Direction::Left)
            velocity.x = -controller.m_rangedSpeed;

        entityManager.SpawnProjectile(
            &shooter,
            transform.GetPosition(),
            velocity,
            controller.m_rangedDamage,
            controller.m_rangedLifetime);
    }
}

void MovementControlSystem::Initialize(EntityManager* entityManager)
{
    m_entityManager = entityManager;
    if (!m_entityManager) return;

    EventManager& events = m_entityManager->GetContext().m_eventManager;

    events.AddCallback(StateType::Game, "Player_MoveLeft", &MovementControlSystem::React, *this);
    events.AddCallback(StateType::Game, "Player_MoveRight", &MovementControlSystem::React, *this);
    events.AddCallback(StateType::Game, "Player_Jump", &MovementControlSystem::React, *this);
    events.AddCallback(StateType::Game, "Player_Jump_Cancel", &MovementControlSystem::React, *this);
    events.AddCallback(StateType::Game, "Player_Attack", &MovementControlSystem::React, *this);
    events.AddCallback(StateType::Game, "Attack_Ranged", &MovementControlSystem::React, *this);
}

void MovementControlSystem::Destroy()
{
    if (!m_entityManager) return;
    EventManager& events = m_entityManager->GetContext().m_eventManager;

    events.RemoveCallback(StateType::Game, "Player_MoveLeft");
    events.RemoveCallback(StateType::Game, "Player_MoveRight");
    events.RemoveCallback(StateType::Game, "Player_Jump");
    events.RemoveCallback(StateType::Game, "Player_Jump_Cancel");
    events.RemoveCallback(StateType::Game, "Player_Attack");
    events.RemoveCallback(StateType::Game, "Attack_Ranged");
}

void MovementControlSystem::React(EventDetails& details)
{
    if (!m_entityManager) return;
    std::string action = details.m_name;

    // Route the input to any entity that is a Player and has a Controller
    for (auto& entityPair : m_entityManager->GetEntities())
    {
        EntityBase* entity = entityPair.second.get();

        if (entity->GetType() == EntityType::Player)
        {
            CController* controller = entity->GetComponent<CController>();
            if (!controller) continue;

            if (action == "Player_MoveLeft")            controller->m_moveLeft = true;
            else if (action == "Player_MoveRight")      controller->m_moveRight = true;
            else if (action == "Player_Jump")           controller->m_jump = true;
            else if (action == "Player_Jump_Cancel")    controller->m_cancelJump = true;
            else if (action == "Player_Attack")         controller->m_attack = true;
            else if (action == "Attack_Ranged")         controller->m_attackRanged = true;
        }
    }
}

void MovementControlSystem::Update(float deltaTime)
{
    if (!m_entityManager) return;

    for (auto& entityPair : m_entityManager->GetEntities())
    {
        EntityBase* entity = entityPair.second.get();
        CController* controller = entity->GetComponent<CController>();
        CTransform* transform = entity->GetComponent<CTransform>();
        CState* state = entity->GetComponent<CState>();
        CSprite* sprite = entity->GetComponent<CSprite>();
        CBoxCollider* collider = entity->GetComponent<CBoxCollider>();

        // Only process controllable entities
        if (!controller || !transform || !state) continue;

        const EntityState currentState = state->GetState();
        if (currentState == EntityState::Dying) continue;

        // Timers
        DecreaseToZero(controller->m_attackCooldownTimer, deltaTime);
        DecreaseToZero(controller->m_jumpBufferTimer, deltaTime);
        DecreaseToZero(controller->m_rangedCooldownTimer, deltaTime);
        // Coyote timer logic
        const bool grounded = IsGrounded(collider);
        if (grounded)
            controller->m_coyoteTimer = controller->m_coyoteTimeWindow;
        else
            DecreaseToZero(controller->m_coyoteTimer, deltaTime);

        // Movement
        if (CanMove(currentState))
        {
            if (controller->m_moveLeft)
            {
                if (sprite) sprite->SetDirection(Direction::Left);
                transform->AddAcceleration(-transform->GetSpeed().x, 0.0f);
                if (currentState == EntityState::Idle) state->SetState(EntityState::Walking);
            }
            else if (controller->m_moveRight)
            {
                if (sprite) sprite->SetDirection(Direction::Right);
                transform->AddAcceleration(transform->GetSpeed().x, 0.0f);
                if (currentState == EntityState::Idle) state->SetState(EntityState::Walking);
            }
        }

        // Jump input buffer
        if (controller->m_jump)
        {
            const EntityState jumpState = state->GetState();
            if (CanQueueJump(jumpState))
                controller->m_jumpBufferTimer = controller->m_jumpBufferWindow;

            controller->m_jump = false; // Consume the input
        }

        // Execute buffered jump
        const bool canConsumeBufferedJump =
            controller->m_jumpBufferTimer > 0.0f &&
            (grounded || controller->m_coyoteTimer > 0.0f) &&
            CanQueueJump(state->GetState());

        if (canConsumeBufferedJump)
        {
            controller->m_jumpBufferTimer = 0.0f;
            controller->m_coyoteTimer = 0.0f;
            state->SetState(EntityState::Jumping);
            transform->SetVelocity(transform->GetVelocity().x, -controller->m_jumpVelocity);
        }

        if (controller->m_cancelJump)
        {
            if (transform->GetVelocity().y < 0.0f)
                transform->SetVelocity(transform->GetVelocity().x, transform->GetVelocity().y * controller->m_jumpCancelMultiplier);

            controller->m_cancelJump = false; // Consume the input
        }

        // Attack
        // Attack (melee)
        bool meleeTriggered = false;
        if (controller->m_attack)
        {
            const EntityState attackState = state->GetState();
            if (CanStartMeleeAttack(attackState, controller->m_attackCooldownTimer))
            {
                transform->SetAcceleration(0.0f, 0.0f);
                transform->SetVelocity(0.0f, 0.0f);
                state->SetState(EntityState::Attacking);
                controller->m_attackCooldownTimer = controller->m_attackCooldown;
                meleeTriggered = true;
            }

            controller->m_attack = false; // Consume the input
        }

        // Ranged attack (same pipeline as melee)
        if (controller->m_attackRanged)
        {
            const EntityState rangedState = state->GetState();
            if (!meleeTriggered &&
                sprite &&
                CanStartRangedAttack(rangedState, controller->m_rangedCooldownTimer))
            {
                SpawnRangedProjectile(*m_entityManager, *entity, *transform, *sprite, *controller);
                controller->m_rangedCooldownTimer = controller->m_rangedCooldown;
            }

            controller->m_attackRanged = false; // Consume the input
        }

        // Auto state cleanup
        const EntityState updatedState = state->GetState();
        if (CanAutoState(updatedState))
            state->SetState(ResolveLocomotionState(transform->GetVelocity(), controller->m_verticalAirThreshold, controller->m_horizontalWalkThreshold));

        // Reset continuous movement flags
        controller->m_moveLeft = false;
        controller->m_moveRight = false;
    }
}