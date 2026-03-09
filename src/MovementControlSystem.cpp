#include <cmath>
#include <vector>
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

    struct ProjectileSpawnRequest
    {
        unsigned int shooterId = 0;
        sf::Vector2f position = { 0.0f, 0.0f };
        sf::Vector2f velocity = { 0.0f, 0.0f };
        int damage = 0;
        float lifetime = 0.0f;
    };

    inline ProjectileSpawnRequest BuildRangedProjectileRequest(
        const EntityBase& shooter,
        const CTransform& transform,
        const CSprite& sprite,
        const CController& controller,
        const CBoxCollider* collider)
    {
        ProjectileSpawnRequest request;
        request.shooterId = shooter.GetId();

        const bool facingLeft = (sprite.GetDirection() == Direction::Left);
        const float dirSign = facingLeft ? -1.0f : 1.0f;

        request.velocity = { dirSign * controller.m_rangedSpeed, 0.0f };

        request.position = transform.GetPosition();
        request.position.x += dirSign * controller.m_rangedSpawnOffsetX;
        request.position.y += controller.m_rangedSpawnOffsetY;

        if (collider)
        {
            const sf::FloatRect body = collider->GetAABB();

            float halfW = controller.m_rangedSizeX * 0.5f;
            if (halfW < 0.0f) halfW = 0.0f;

            float halfH = controller.m_rangedSizeY * 0.5f;
            if (halfH < 0.0f) halfH = 0.0f;

            // Keep spawn vertically inside the body column to avoid floor overlap
            const float minY = body.position.y + halfH + 1.0f;
            const float maxY = body.position.y + body.size.y - halfH - 1.0f;
            if (request.position.y < minY) request.position.y = minY;
            if (request.position.y > maxY) request.position.y = maxY;

            // Push spawn just outside the body on facing side
            if (facingLeft)
            {
                const float maxX = body.position.x - halfW - 1.0f;
                if (request.position.x > maxX) request.position.x = maxX;
            }
            else
            {
                const float minX = body.position.x + body.size.x + halfW + 1.0f;
                if (request.position.x < minX) request.position.x = minX;
            }
        }

        request.damage = controller.m_rangedDamage;
        request.lifetime = controller.m_rangedLifetime;
        return request;
    }

    enum class HorizontalIntent
    {
        None,
        Left,
        Right
    };

    inline HorizontalIntent ResolveHorizontalIntent(bool moveLeft, bool moveRight)
    {
        // Explicit conflict policy: opposite directions cancel each other.
        if (moveLeft == moveRight) return HorizontalIntent::None;
        return moveLeft ? HorizontalIntent::Left : HorizontalIntent::Right;
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

    // This input path is intentionally single-player
    // We resolve the player directly instead of scanning all entities
    EntityBase* player = m_entityManager->GetPlayer();
    if (!player) return;

    CController* controller = player->GetComponent<CController>();
    if (!controller) return;

    const std::string& action = details.m_name;

    if (action == "Player_MoveLeft")           controller->m_moveLeft = true;
    else if (action == "Player_MoveRight")     controller->m_moveRight = true;
    else if (action == "Player_Jump")          controller->m_jump = true;
    else if (action == "Player_Jump_Cancel")   controller->m_cancelJump = true;
    else if (action == "Player_Attack")        controller->m_attack = true;
    else if (action == "Attack_Ranged" && controller->m_rangedEnabled) controller->m_attackRanged = true;
}

void MovementControlSystem::Update(float deltaTime)
{
    if (!m_entityManager) return;

    std::vector<ProjectileSpawnRequest> pendingProjectiles;

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

        const HorizontalIntent horizontalIntent =
            ResolveHorizontalIntent(controller->m_moveLeft, controller->m_moveRight);

        // Movement
        if (CanMove(currentState))
        {
            if (horizontalIntent == HorizontalIntent::Left)
            {
                if (sprite) sprite->SetDirection(Direction::Left);
                transform->AddAcceleration(-transform->GetSpeed().x, 0.0f);
                if (currentState == EntityState::Idle) state->SetState(EntityState::Walking);
            }
            else if (horizontalIntent == HorizontalIntent::Right)
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
                controller->m_rangedEnabled &&
                sprite &&
                CanStartRangedAttack(rangedState, controller->m_rangedCooldownTimer))
            {
                pendingProjectiles.push_back(
                    BuildRangedProjectileRequest(*entity, *transform, *sprite, *controller, collider));

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

    for (const ProjectileSpawnRequest& request : pendingProjectiles)
    {
        EntityBase* shooter = m_entityManager->Find(request.shooterId);
        if (!shooter) continue;

        m_entityManager->SpawnProjectile(
            shooter,
            request.position,
            request.velocity,
            request.damage,
            request.lifetime);
    }
}