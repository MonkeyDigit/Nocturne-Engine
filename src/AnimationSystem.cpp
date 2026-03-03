#include "AnimationSystem.h"
#include "EntityManager.h"
#include "CSprite.h"
#include "CState.h"
#include "CTransform.h"

void AnimationSystem::Update(EntityManager& entityManager, float deltaTime)
{
    for (auto& entityPair : entityManager.GetEntities())
    {
        EntityBase* entity = entityPair.second.get();

        CSprite* sprite = entity->GetComponent<CSprite>();
        CState* stateComp = entity->GetComponent<CState>();
        CTransform* transform = entity->GetComponent<CTransform>();

        // We only process animations for entities with a sprite and a state
        if (!sprite || !stateComp) continue;

        EntityState state = stateComp->GetState();
        Animation* currentAnimation = sprite->GetSpriteSheet().GetCurrentAnim();

        if (!currentAnimation) continue;

        std::string animName = currentAnimation->GetName();

        // --- ADVANCED PLAYER ANIMATIONS ---
        if (entity->GetType() == EntityType::Player && transform)
        {
            if (state == EntityState::Walking && animName != "Walk")
            {
                if ((animName == "WalkStart" && !currentAnimation->IsPlaying()) ||
                    animName == "Jump" || animName == "Fall")
                {
                    sprite->GetSpriteSheet().SetAnimation("Walk", true, true);
                }
                else if (animName != "WalkStart")
                {
                    sprite->GetSpriteSheet().SetAnimation("WalkStart", true, false);
                }
            }
            else if (state == EntityState::Jumping)
            {
                if (transform->GetVelocity().y > 0.0f && animName != "Fall")
                    sprite->GetSpriteSheet().SetAnimation("Fall", true, false);
                else if (transform->GetVelocity().x != 0.0f && transform->GetVelocity().y < 0.0f && animName != "JumpForward")
                    sprite->GetSpriteSheet().SetAnimation("JumpForward", true, false);
                else if (transform->GetVelocity().y < 0.0f && transform->GetVelocity().x == 0.0f &&
                    animName != "Jump" && animName != "JumpForward")
                    sprite->GetSpriteSheet().SetAnimation("Jump", true, false);
            }
            else if (state == EntityState::Attacking && animName != "Attack")
                sprite->GetSpriteSheet().SetAnimation("Attack", true, false);
            else if (state == EntityState::Hurt && animName != "Hurt")
                sprite->GetSpriteSheet().SetAnimation("Hurt", true, false);
            else if (state == EntityState::Dying && animName != "Death")
                sprite->GetSpriteSheet().SetAnimation("Death", true, false);
            else if (state == EntityState::Idle && animName != "Idle")
            {
                if (animName == "Walk") sprite->GetSpriteSheet().SetAnimation("WalkEnd", true, false);
                else if (animName == "Fall") sprite->GetSpriteSheet().SetAnimation("Land", true, false);

                currentAnimation = sprite->GetSpriteSheet().GetCurrentAnim();
                if ((currentAnimation->GetName() == "WalkEnd" && currentAnimation->IsPlaying()) ||
                    (currentAnimation->GetName() == "Land" && currentAnimation->IsPlaying())) { /* Wait */
                }
                else sprite->GetSpriteSheet().SetAnimation("Idle", true, true);
            }
        }
        // --- BASIC ENEMY/CHARACTER ANIMATIONS ---
        else
        {
            if (state == EntityState::Walking && animName != "Walk")
                sprite->GetSpriteSheet().SetAnimation("Walk", true, true);
            else if (state == EntityState::Jumping && animName != "Jump")
                sprite->GetSpriteSheet().SetAnimation("Jump", true, false);
            else if (state == EntityState::Attacking && animName != "Attack")
                sprite->GetSpriteSheet().SetAnimation("Attack", true, false);
            else if (state == EntityState::Hurt && animName != "Hurt")
                sprite->GetSpriteSheet().SetAnimation("Hurt", true, false);
            else if (state == EntityState::Dying && animName != "Death")
                sprite->GetSpriteSheet().SetAnimation("Death", true, false);
            else if (state == EntityState::Idle && animName != "Idle")
                sprite->GetSpriteSheet().SetAnimation("Idle", true, true);
        }

        // Update the animation timer
        sprite->Update(deltaTime);
    }
}