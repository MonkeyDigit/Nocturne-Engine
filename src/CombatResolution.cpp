#include "CombatSystem.h"
#include "CombatGeometry.h"
#include "EntityBase.h"
#include "CBoxCollider.h"
#include "CTransform.h"
#include "CState.h"
#include "CSprite.h"
#include "CProjectile.h"
#include "GameplayTuning.h"

namespace
{
    inline bool HasIntersection(const sf::FloatRect& a, const sf::FloatRect& b)
    {
        return a.findIntersection(b).has_value();
    }

    inline void ApplyKnockbackAwayFrom(
        CTransform& target,
        const CTransform& source,
        float horizontalForce,
        float verticalForce)
    {
        const float direction = (source.GetPosition().x < target.GetPosition().x) ? 1.0f : -1.0f;
        target.AddVelocity(direction * horizontalForce, verticalForce);
    }

    inline void FaceTarget(CSprite& sprite, const CTransform& from, const CTransform& to)
    {
        sprite.SetDirection((from.GetPosition().x > to.GetPosition().x) ? Direction::Left : Direction::Right);
    }

    enum class ProjectileHitPolicy
    {
        ConsumeAlways,
        ConsumeOnDamage
    };

    constexpr ProjectileHitPolicy kProjectileHitPolicy = ProjectileHitPolicy::ConsumeOnDamage;

    inline bool ShouldConsumeProjectile(bool damageApplied)
    {
        return (kProjectileHitPolicy == ProjectileHitPolicy::ConsumeAlways) || damageApplied;
    }
}

void CombatSystem::ResolveProjectiles(
    const std::vector<EntityBase*>& projectiles,
    const std::vector<EntityBase*>& targets)
{
    for (EntityBase* projEntity : projectiles)
    {
        CProjectile* projComp = projEntity->GetComponent<CProjectile>();
        CBoxCollider* projCol = projEntity->GetComponent<CBoxCollider>();
        if (!projComp || !projCol) continue;

        const sf::FloatRect projAABB = projCol->GetAABB();

        for (EntityBase* target : targets)
        {
            if (target->GetType() == EntityType::Projectile) continue;
            if (target->GetType() == projComp->GetShooterType()) continue;

            CState* targetState = target->GetComponent<CState>();
            CBoxCollider* targetCol = target->GetComponent<CBoxCollider>();
            if (!targetState || !targetCol || targetState->GetState() == EntityState::Dying) continue;

            if (!HasIntersection(projAABB, targetCol->GetAABB()))
                continue;

            const bool damageApplied = targetState->TakeDamage(projComp->GetDamage());
            if (ShouldConsumeProjectile(damageApplied))
            {
                projEntity->Destroy();
                break;
            }
        }
    }
}

void CombatSystem::ResolveEnemyVsPlayer(
    EntityBase* player,
    const std::vector<EntityBase*>& enemies,
    const GameplayTuning& tuning)
{
    if (!player) return;

    CState* playerState = player->GetComponent<CState>();
    CTransform* playerTrans = player->GetComponent<CTransform>();
    CBoxCollider* playerCol = player->GetComponent<CBoxCollider>();
    CSprite* playerSprite = player->GetComponent<CSprite>();

    if (!playerState || !playerTrans || !playerCol || !playerSprite) return;
    if (playerState->GetState() == EntityState::Dying) return;

    const bool playerIsAttacking = (playerState->GetState() == EntityState::Attacking);
    const unsigned int playerAttackInstance = playerIsAttacking ? playerState->GetAttackInstance() : 0u;
    const sf::FloatRect playerSwordBox = playerIsAttacking
        ? ComputeWorldAttackAABB(playerCol, playerSprite)
        : sf::FloatRect();

    for (EntityBase* enemy : enemies)
    {
        CState* enemyState = enemy->GetComponent<CState>();
        CTransform* enemyTrans = enemy->GetComponent<CTransform>();
        CBoxCollider* enemyCol = enemy->GetComponent<CBoxCollider>();
        CSprite* enemySprite = enemy->GetComponent<CSprite>();

        if (!enemyState || !enemyTrans || !enemyCol || !enemySprite) continue;
        if (enemyState->GetState() == EntityState::Dying) continue;

        // Player sword hit
        if (playerIsAttacking &&
            HasIntersection(playerSwordBox, enemyCol->GetAABB()) &&
            enemyState->TakeDamageFromAttack(
                player->GetId(),
                playerAttackInstance,
                playerState->GetAttackDamage()))
        {
            const float knockbackX = playerState->HasAttackKnockbackOverride()
                ? playerState->GetAttackKnockbackX()
                : tuning.m_playerSwordKnockbackX;

            const float knockbackY = playerState->HasAttackKnockbackOverride()
                ? playerState->GetAttackKnockbackY()
                : tuning.m_playerSwordKnockbackY;

            ApplyKnockbackAwayFrom(*enemyTrans, *playerTrans, knockbackX, knockbackY);
        }

        // Enemy active attack hitbox
        if (enemyState->GetState() == EntityState::Attacking)
        {
            const unsigned int enemyAttackInstance = enemyState->GetAttackInstance();
            const sf::FloatRect enemyAttackBox = ComputeWorldAttackAABB(enemyCol, enemySprite);

            if (HasIntersection(enemyAttackBox, playerCol->GetAABB()) &&
                playerState->TakeDamageFromAttack(
                    enemy->GetId(),
                    enemyAttackInstance,
                    enemyState->GetAttackDamage()))
            {
                const float knockbackX = enemyState->HasAttackKnockbackOverride()
                    ? enemyState->GetAttackKnockbackX()
                    : tuning.m_enemyAttackKnockbackX;

                const float knockbackY = enemyState->HasAttackKnockbackOverride()
                    ? enemyState->GetAttackKnockbackY()
                    : tuning.m_enemyAttackKnockbackY;

                ApplyKnockbackAwayFrom(*playerTrans, *enemyTrans, knockbackX, knockbackY);
                FaceTarget(*enemySprite, *enemyTrans, *playerTrans);
            }

            continue;
        }

        // Passive body-contact damage
        if (!HasIntersection(enemyCol->GetAABB(), playerCol->GetAABB()))
            continue;

        const int touchDamage = enemyState->GetTouchDamage();
        if (touchDamage > 0)
            playerState->TakeDamage(touchDamage);
    }
}