#include <cmath>
#include <string>

#include "AnimationSystem.h"
#include "EntityManager.h"
#include "CSprite.h"
#include "CState.h"
#include "CTransform.h"

namespace
{
    constexpr float kVelocityEpsilon = 0.01f;

    inline bool IsFinite(float value)
    {
        return std::isfinite(value);
    }

    inline bool IsMovingHorizontally(const sf::Vector2f& velocity)
    {
        return IsFinite(velocity.x) && std::abs(velocity.x) > kVelocityEpsilon;
    }

    inline bool IsRising(const sf::Vector2f& velocity)
    {
        return IsFinite(velocity.y) && velocity.y < -kVelocityEpsilon;
    }

    inline bool IsFalling(const sf::Vector2f& velocity)
    {
        return IsFinite(velocity.y) && velocity.y > kVelocityEpsilon;
    }

    inline bool TrySetAnimationIfNeeded(SpriteSheet& sheet, const char* name, bool play, bool loop)
    {
        if (sheet.IsCurrentAnimation(name))
            return true;

        return sheet.SetAnimation(name, play, loop);
    }

    inline void HandleDyingState(EntityBase& entity, EntityManager& entityManager, SpriteSheet& sheet)
    {
        // Dying is terminal: enforce Death animation, then remove entity on completion
        if (!TrySetAnimationIfNeeded(sheet, "Death", true, false))
        {
            entityManager.Remove(entity.GetId());
            return;
        }

        Animation* deathAnim = sheet.GetCurrentAnim();
        if (!deathAnim || deathAnim->GetName() != "Death")
        {
            entityManager.Remove(entity.GetId());
            return;
        }

        if (!deathAnim->IsPlaying())
            entityManager.Remove(entity.GetId());
    }

    inline void HandleOneShotState(CState& stateComp, SpriteSheet& sheet)
    {
        const EntityState state = stateComp.GetState();
        if (state != EntityState::Attacking && state != EntityState::Hurt) return;

        const char* oneShotName = (state == EntityState::Attacking) ? "Attack" : "Hurt";

        // One-shot states should either play or fail back to Idle
        if (!TrySetAnimationIfNeeded(sheet, oneShotName, true, false))
        {
            stateComp.SetState(EntityState::Idle);
            return;
        }

        Animation* oneShotAnim = sheet.GetCurrentAnim();
        if (oneShotAnim &&
            oneShotAnim->GetName() == oneShotName &&
            !oneShotAnim->IsPlaying())
        {
            stateComp.SetState(EntityState::Idle);
        }
    }

    inline Animation* EnsureCurrentAnimation(SpriteSheet& sheet)
    {
        Animation* current = sheet.GetCurrentAnim();
        if (current) return current;

        if (!TrySetAnimationIfNeeded(sheet, "Idle", true, true))
            return nullptr;

        return sheet.GetCurrentAnim();
    }

    inline void HandlePlayerWalking(SpriteSheet& sheet, const sf::Vector2f& velocity)
    {
        Animation* currentAnimation = sheet.GetCurrentAnim();
        if (!currentAnimation) return;

        const std::string animName = currentAnimation->GetName();

        if (animName == "WalkStart")
        {
            if (!currentAnimation->IsPlaying())
                TrySetAnimationIfNeeded(sheet, "Walk", true, true);
            return;
        }

        if (animName == "Jump" || animName == "JumpForward")
        {
            // Avoid apex flicker: do not force Walk while jump transition is still valid
            if (IsFalling(velocity))
            {
                TrySetAnimationIfNeeded(sheet, "Fall", true, false);
            }
            else if (!currentAnimation->IsPlaying())
            {
                TrySetAnimationIfNeeded(sheet, "Walk", true, true);
            }
            return;
        }

        if (animName == "Fall")
        {
            // Leave Fall only when descent is over (landing)
            if (!IsFalling(velocity))
                TrySetAnimationIfNeeded(sheet, "Walk", true, true);
            return;
        }

        if (animName != "Walk")
            TrySetAnimationIfNeeded(sheet, "WalkStart", true, false);
    }

    inline void HandlePlayerJumping(SpriteSheet& sheet, const sf::Vector2f& velocity)
    {
        if (IsFalling(velocity))
        {
            TrySetAnimationIfNeeded(sheet, "Fall", true, false);
            return;
        }

        if (IsRising(velocity) && IsMovingHorizontally(velocity))
        {
            TrySetAnimationIfNeeded(sheet, "JumpForward", true, false);
            return;
        }

        if (IsRising(velocity))
            TrySetAnimationIfNeeded(sheet, "Jump", true, false);

        // At apex (|vy| ~ 0), keep current jump/fall animation unchanged
    }

    inline void HandlePlayerIdle(SpriteSheet& sheet)
    {
        Animation* currentAnimation = sheet.GetCurrentAnim();
        if (!currentAnimation) return;

        const std::string animName = currentAnimation->GetName();
        if (animName == "Idle") return;

        if (animName == "Walk")
            TrySetAnimationIfNeeded(sheet, "WalkEnd", true, false);
        else if (animName == "Fall")
            TrySetAnimationIfNeeded(sheet, "Land", true, false);

        currentAnimation = sheet.GetCurrentAnim();
        if (!currentAnimation) return;

        const std::string transition = currentAnimation->GetName();
        const bool transitionPlaying =
            (transition == "WalkEnd" || transition == "Land") &&
            currentAnimation->IsPlaying();

        if (!transitionPlaying)
            TrySetAnimationIfNeeded(sheet, "Idle", true, true);
    }

    inline void HandlePlayerAnimation(
        SpriteSheet& sheet,
        EntityState state,
        const sf::Vector2f& velocity)
    {
        switch (state)
        {
        case EntityState::Walking:
            HandlePlayerWalking(sheet, velocity);
            break;
        case EntityState::Jumping:
            HandlePlayerJumping(sheet, velocity);
            break;
        case EntityState::Idle:
            HandlePlayerIdle(sheet);
            break;
        default:
            break;
        }
    }

    inline void HandleNonPlayerAnimation(SpriteSheet& sheet, EntityState state)
    {
        switch (state)
        {
        case EntityState::Walking:
            TrySetAnimationIfNeeded(sheet, "Walk", true, true);
            break;
        case EntityState::Jumping:
            TrySetAnimationIfNeeded(sheet, "Jump", true, false);
            break;
        case EntityState::Idle:
            TrySetAnimationIfNeeded(sheet, "Idle", true, true);
            break;
        default:
            break;
        }
    }
}

void AnimationSystem::Update(EntityManager& entityManager, float deltaTime)
{
    (void)deltaTime;

    for (auto& entityPair : entityManager.GetEntities())
    {
        if (!entityPair.second) continue;

        EntityBase& entity = *entityPair.second;
        CSprite* sprite = entity.GetComponent<CSprite>();
        CState* stateComp = entity.GetComponent<CState>();
        CTransform* transform = entity.GetComponent<CTransform>();

        if (!sprite || !stateComp || !transform) continue;

        SpriteSheet& sheet = sprite->GetSpriteSheet();
        const sf::Vector2f velocity = transform->GetVelocity();

        if (stateComp->GetState() == EntityState::Dying)
        {
            HandleDyingState(entity, entityManager, sheet);
            continue;
        }

        HandleOneShotState(*stateComp, sheet);

        if (!EnsureCurrentAnimation(sheet))
            continue;

        const EntityState resolvedState = stateComp->GetState();

        if (entity.GetType() == EntityType::Player)
            HandlePlayerAnimation(sheet, resolvedState, velocity);
        else
            HandleNonPlayerAnimation(sheet, resolvedState);
    }
}