#include "AnimationSystem.h"
#include "EntityManager.h"
#include "CSprite.h"
#include "CState.h"
#include "CTransform.h"

void AnimationSystem::Update(EntityManager& entityManager, float deltaTime)
{
    (void)deltaTime;

    for (auto& entityPair : entityManager.GetEntities())
    {
        EntityBase* entity = entityPair.second.get();
        CSprite* sprite = entity->GetComponent<CSprite>();
        CState* stateComp = entity->GetComponent<CState>();
        CTransform* transform = entity->GetComponent<CTransform>();

        if (!sprite || !stateComp || !transform) continue;

        SpriteSheet& sheet = sprite->GetSpriteSheet();
        EntityState state = stateComp->GetState();

        // Dying is terminal: ensure Death animation, then remove entity when done
        if (state == EntityState::Dying)
        {
            Animation* deathAnim = sheet.GetCurrentAnim();
            if (!deathAnim)
            {
                entityManager.Remove(entity->GetId());
                continue;
            }

            if (deathAnim->GetName() != "Death")
            {
                if (!sheet.SetAnimation("Death", true, false))
                    entityManager.Remove(entity->GetId());
                continue;
            }

            if (!deathAnim->IsPlaying())
                entityManager.Remove(entity->GetId());

            continue;
        }

        // Handle one-shot states in a fail-safe way
        if (state == EntityState::Attacking || state == EntityState::Hurt)
        {
            const char* oneShotName = (state == EntityState::Attacking) ? "Attack" : "Hurt";
            Animation* oneShotAnim = sheet.GetCurrentAnim();

            if (!oneShotAnim || oneShotAnim->GetName() != oneShotName)
            {
                if (!sheet.SetAnimation(oneShotName, true, false))
                {
                    // Avoid locking the entity in one-shot state if animation is missing
                    stateComp->SetState(EntityState::Idle);
                }
            }
            else if (!oneShotAnim->IsPlaying())
            {
                stateComp->SetState(EntityState::Idle);
            }
        }

        state = stateComp->GetState();
        Animation* currentAnimation = sheet.GetCurrentAnim();
        if (!currentAnimation) continue;

        const std::string animName = currentAnimation->GetName();

        // Advanced player animation rules
        if (entity->GetType() == EntityType::Player)
        {
            if (state == EntityState::Walking && animName != "Walk")
            {
                if ((animName == "WalkStart" && !currentAnimation->IsPlaying()) ||
                    animName == "Jump" || animName == "Fall")
                {
                    sheet.SetAnimation("Walk", true, true);
                }
                else if (animName != "WalkStart")
                {
                    sheet.SetAnimation("WalkStart", true, false);
                }
            }
            else if (state == EntityState::Jumping)
            {
                if (transform->GetVelocity().y > 0.0f && animName != "Fall")
                    sheet.SetAnimation("Fall", true, false);
                else if (transform->GetVelocity().x != 0.0f &&
                    transform->GetVelocity().y < 0.0f &&
                    animName != "JumpForward")
                    sheet.SetAnimation("JumpForward", true, false);
                else if (transform->GetVelocity().y < 0.0f &&
                    transform->GetVelocity().x == 0.0f &&
                    animName != "Jump" &&
                    animName != "JumpForward")
                    sheet.SetAnimation("Jump", true, false);
            }
            else if (state == EntityState::Idle && animName != "Idle")
            {
                if (animName == "Walk") sheet.SetAnimation("WalkEnd", true, false);
                else if (animName == "Fall") sheet.SetAnimation("Land", true, false);

                currentAnimation = sheet.GetCurrentAnim();
                if (!currentAnimation) continue;

                if ((currentAnimation->GetName() == "WalkEnd" && currentAnimation->IsPlaying()) ||
                    (currentAnimation->GetName() == "Land" && currentAnimation->IsPlaying()))
                {
                    // Keep transition one-shots until completion
                }
                else
                {
                    sheet.SetAnimation("Idle", true, true);
                }
            }
        }
        // Basic enemy/character animation rules
        else
        {
            if (state == EntityState::Walking && animName != "Walk")
                sheet.SetAnimation("Walk", true, true);
            else if (state == EntityState::Jumping && animName != "Jump")
                sheet.SetAnimation("Jump", true, false);
            else if (state == EntityState::Idle && animName != "Idle")
                sheet.SetAnimation("Idle", true, true);
        }
    }
}