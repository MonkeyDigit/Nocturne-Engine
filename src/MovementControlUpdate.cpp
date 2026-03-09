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
        const CBoxCollider* collider,
        const GameplayTuning& tuning)
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

            const float projectileSizeX = (controller.m_rangedSizeX > 0.0f)
                ? controller.m_rangedSizeX
                : tuning.m_projectileFallbackWidth;

            const float projectileSizeY = (controller.m_rangedSizeY > 0.0f)
                ? controller.m_rangedSizeY
                : tuning.m_projectileFallbackHeight;

            float halfW = projectileSizeX * 0.5f;
            if (halfW < 0.0f) halfW = 0.0f;

            float halfH = projectileSizeY * 0.5f;
            if (halfH < 0.0f) halfH = 0.0f;

            // Keep spawn vertically inside the body column to avoid floor overlap.
            const float minY = body.position.y + halfH + 1.0f;
            const float maxY = body.position.y + body.size.y - halfH - 1.0f;
            if (request.position.y < minY) request.position.y = minY;
            if (request.position.y > maxY) request.position.y = maxY;

            // Push spawn just outside the body on facing side.
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

    inline bool TryGetControllableComponents(
        EntityBase& entity,
        CController*& outController,
        CTransform*& outTransform,
        CState*& outState,
        CSprite*& outSprite,
        CBoxCollider*& outCollider)
    {
        outController = entity.GetComponent<CController>();
        outTransform = entity.GetComponent<CTransform>();
        outState = entity.GetComponent<CState>();
        outSprite = entity.GetComponent<CSprite>();
        outCollider = entity.GetComponent<CBoxCollider>();

        // Controller logic requires controller + transform + state.
        return outController && outTransform && outState;
    }

    inline void UpdateTimersAndGrounding(CController& controller, float deltaTime, bool grounded)
    {
        DecreaseToZero(controller.m_attackCooldownTimer, deltaTime);
        DecreaseToZero(controller.m_jumpBufferTimer, deltaTime);
        DecreaseToZero(controller.m_rangedCooldownTimer, deltaTime);

        if (grounded)
            controller.m_coyoteTimer = controller.m_coyoteTimeWindow;
        else
            DecreaseToZero(controller.m_coyoteTimer, deltaTime);
    }

    inline void HandleHorizontalMovement(
        HorizontalIntent horizontalIntent,
        EntityState currentState,
        CTransform& transform,
        CState& state,
        CSprite* sprite)
    {
        if (!CanMove(currentState)) return;

        if (horizontalIntent == HorizontalIntent::Left)
        {
            if (sprite) sprite->SetDirection(Direction::Left);
            transform.AddAcceleration(-transform.GetSpeed().x, 0.0f);
            if (currentState == EntityState::Idle) state.SetState(EntityState::Walking);
        }
        else if (horizontalIntent == HorizontalIntent::Right)
        {
            if (sprite) sprite->SetDirection(Direction::Right);
            transform.AddAcceleration(transform.GetSpeed().x, 0.0f);
            if (currentState == EntityState::Idle) state.SetState(EntityState::Walking);
        }
    }

    inline void HandleJumpInput(
        CController& controller,
        CTransform& transform,
        CState& state,
        bool grounded)
    {
        if (controller.m_jump)
        {
            const EntityState jumpState = state.GetState();
            if (CanQueueJump(jumpState))
                controller.m_jumpBufferTimer = controller.m_jumpBufferWindow;

            controller.m_jump = false;
        }

        const bool canConsumeBufferedJump =
            controller.m_jumpBufferTimer > 0.0f &&
            (grounded || controller.m_coyoteTimer > 0.0f) &&
            CanQueueJump(state.GetState());

        if (canConsumeBufferedJump)
        {
            controller.m_jumpBufferTimer = 0.0f;
            controller.m_coyoteTimer = 0.0f;
            state.SetState(EntityState::Jumping);
            transform.SetVelocity(transform.GetVelocity().x, -controller.m_jumpVelocity);
        }

        if (controller.m_cancelJump)
        {
            if (transform.GetVelocity().y < 0.0f)
            {
                transform.SetVelocity(
                    transform.GetVelocity().x,
                    transform.GetVelocity().y * controller.m_jumpCancelMultiplier);
            }

            controller.m_cancelJump = false;
        }
    }

    inline bool HandleMeleeInput(CController& controller, CTransform& transform, CState& state)
    {
        bool meleeTriggered = false;

        if (controller.m_attack)
        {
            const EntityState attackState = state.GetState();
            if (CanStartMeleeAttack(attackState, controller.m_attackCooldownTimer))
            {
                transform.SetAcceleration(0.0f, 0.0f);
                transform.SetVelocity(0.0f, 0.0f);
                state.SetState(EntityState::Attacking);
                controller.m_attackCooldownTimer = controller.m_attackCooldown;
                meleeTriggered = true;
            }

            controller.m_attack = false;
        }

        return meleeTriggered;
    }

    inline void HandleRangedInput(
        const EntityBase& entity,
        CController& controller,
        const CTransform& transform,
        CState& state,
        const CSprite* sprite,
        const CBoxCollider* collider,
        const GameplayTuning& tuning,
        bool meleeTriggered,
        std::vector<ProjectileSpawnRequest>& pendingProjectiles)
    {
        if (controller.m_attackRanged)
        {
            const EntityState rangedState = state.GetState();
            if (!meleeTriggered &&
                controller.m_rangedEnabled &&
                sprite &&
                CanStartRangedAttack(rangedState, controller.m_rangedCooldownTimer))
            {
                pendingProjectiles.push_back(
                    BuildRangedProjectileRequest(entity, transform, *sprite, controller, collider, tuning));

                controller.m_rangedCooldownTimer = controller.m_rangedCooldown;
            }

            controller.m_attackRanged = false;
        }
    }

    inline void UpdateAutoState(CController& controller, CTransform& transform, CState& state)
    {
        const EntityState updatedState = state.GetState();
        if (CanAutoState(updatedState))
        {
            state.SetState(
                ResolveLocomotionState(
                    transform.GetVelocity(),
                    controller.m_verticalAirThreshold,
                    controller.m_horizontalWalkThreshold));
        }
    }

    inline void ResetDirectionalInput(CController& controller)
    {
        controller.m_moveLeft = false;
        controller.m_moveRight = false;
    }

    inline void UpdateSingleControllableEntity(
        EntityBase& entity,
        const GameplayTuning& tuning,
        float deltaTime,
        std::vector<ProjectileSpawnRequest>& pendingProjectiles)
    {
        CController* controller = nullptr;
        CTransform* transform = nullptr;
        CState* state = nullptr;
        CSprite* sprite = nullptr;
        CBoxCollider* collider = nullptr;

        if (!TryGetControllableComponents(entity, controller, transform, state, sprite, collider))
            return;

        const EntityState currentState = state->GetState();
        if (currentState == EntityState::Dying)
            return;

        const bool grounded = IsGrounded(collider);
        UpdateTimersAndGrounding(*controller, deltaTime, grounded);

        const HorizontalIntent horizontalIntent =
            ResolveHorizontalIntent(controller->m_moveLeft, controller->m_moveRight);

        HandleHorizontalMovement(horizontalIntent, currentState, *transform, *state, sprite);
        HandleJumpInput(*controller, *transform, *state, grounded);

        const bool meleeTriggered = HandleMeleeInput(*controller, *transform, *state);

        HandleRangedInput(
            entity,
            *controller,
            *transform,
            *state,
            sprite,
            collider,
            tuning,
            meleeTriggered,
            pendingProjectiles);

        UpdateAutoState(*controller, *transform, *state);
        ResetDirectionalInput(*controller);
    }

    inline void SpawnPendingProjectiles(
        EntityManager& entityManager,
        const std::vector<ProjectileSpawnRequest>& pendingProjectiles)
    {
        for (const ProjectileSpawnRequest& request : pendingProjectiles)
        {
            EntityBase* shooter = entityManager.Find(request.shooterId);
            if (!shooter) continue;

            entityManager.SpawnProjectile(
                shooter,
                request.position,
                request.velocity,
                request.damage,
                request.lifetime);
        }
    }
}

void MovementControlSystem::Update(float deltaTime)
{
    if (!m_entityManager) return;

    std::vector<ProjectileSpawnRequest> pendingProjectiles;
    const GameplayTuning& tuning = m_entityManager->GetContext().m_gameplayTuning;

    for (auto& entityPair : m_entityManager->GetEntities())
    {
        if (!entityPair.second) continue;
        UpdateSingleControllableEntity(*entityPair.second, tuning, deltaTime, pendingProjectiles);
    }

    SpawnPendingProjectiles(*m_entityManager, pendingProjectiles);
}