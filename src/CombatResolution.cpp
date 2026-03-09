#include <cmath>

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
    inline bool IsFinite(float value)
    {
        return std::isfinite(value);
    }

    inline bool IsFiniteVec2(const sf::Vector2f& value)
    {
        return IsFinite(value.x) && IsFinite(value.y);
    }

    inline bool IsValidRect(const sf::FloatRect& rect)
    {
        return IsFiniteVec2(rect.position) &&
            IsFiniteVec2(rect.size) &&
            rect.size.x > 0.0f &&
            rect.size.y > 0.0f;
    }

    inline bool HasIntersection(const sf::FloatRect& a, const sf::FloatRect& b)
    {
        // Ignore invalid rectangles to avoid undefined combat behavior
        if (!IsValidRect(a) || !IsValidRect(b)) return false;
        return a.findIntersection(b).has_value();
    }

    inline float SanitizeKnockbackX(float value)
    {
        // X knockback is treated as magnitude only
        return (IsFinite(value) && value > 0.0f) ? value : 0.0f;
    }

    inline float SanitizeKnockbackY(float value)
    {
        // Y knockback can be positive or negative, but must be finite
        return IsFinite(value) ? value : 0.0f;
    }

    inline void ApplyKnockbackAwayFrom(
        CTransform& target,
        const CTransform& source,
        float horizontalForce,
        float verticalForce)
    {
        const sf::Vector2f sourcePos = source.GetPosition();
        const sf::Vector2f targetPos = target.GetPosition();
        if (!IsFiniteVec2(sourcePos) || !IsFiniteVec2(targetPos)) return;

        const float safeX = SanitizeKnockbackX(horizontalForce);
        const float safeY = SanitizeKnockbackY(verticalForce);
        if (safeX == 0.0f && safeY == 0.0f) return;

        const float direction = (sourcePos.x < targetPos.x) ? 1.0f : -1.0f;
        target.AddVelocity(direction * safeX, safeY);
    }

    inline void FaceTarget(CSprite& sprite, const CTransform& from, const CTransform& to)
    {
        const sf::Vector2f fromPos = from.GetPosition();
        const sf::Vector2f toPos = to.GetPosition();
        if (!IsFiniteVec2(fromPos) || !IsFiniteVec2(toPos)) return;

        sprite.SetDirection((fromPos.x > toPos.x) ? Direction::Left : Direction::Right);
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
        if (!projEntity) continue;

        CProjectile* projComp = projEntity->GetComponent<CProjectile>();
        CBoxCollider* projCol = projEntity->GetComponent<CBoxCollider>();
        if (!projComp || !projCol) continue;

        const int projectileDamage = projComp->GetDamage();
        if (projectileDamage <= 0) continue;

        const sf::FloatRect projAABB = projCol->GetAABB();
        if (!IsValidRect(projAABB)) continue;

        for (EntityBase* target : targets)
        {
            if (!target) continue;
            if (target->GetType() == EntityType::Projectile) continue;
            if (target->GetType() == projComp->GetShooterType()) continue;

            CState* targetState = target->GetComponent<CState>();
            CBoxCollider* targetCol = target->GetComponent<CBoxCollider>();
            if (!targetState || !targetCol || targetState->GetState() == EntityState::Dying) continue;

            const sf::FloatRect targetAABB = targetCol->GetAABB();
            if (!HasIntersection(projAABB, targetAABB))
                continue;

            const bool damageApplied = targetState->TakeDamage(projectileDamage);
            if (ShouldConsumeProjectile(damageApplied))
            {
                projEntity->DestroyAndDisableProjectileDamage();
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

    const sf::FloatRect playerBodyAABB = playerCol->GetAABB();
    if (!IsValidRect(playerBodyAABB)) return;

    const bool playerIsAttacking = (playerState->GetState() == EntityState::Attacking);
    const bool playerCanDealAttackHit = playerIsAttacking && (playerState->GetAttackDamage() > 0);

    const unsigned int playerAttackInstance = playerCanDealAttackHit
        ? playerState->GetAttackInstance()
        : 0u;

    sf::FloatRect playerSwordBox;
    bool hasValidPlayerSwordBox = false;

    if (playerCanDealAttackHit)
    {
        playerSwordBox = ComputeWorldAttackAABB(playerCol, playerSprite);
        hasValidPlayerSwordBox = IsValidRect(playerSwordBox);
    }

    for (EntityBase* enemy : enemies)
    {
        if (!enemy) continue;

        CState* enemyState = enemy->GetComponent<CState>();
        CTransform* enemyTrans = enemy->GetComponent<CTransform>();
        CBoxCollider* enemyCol = enemy->GetComponent<CBoxCollider>();
        CSprite* enemySprite = enemy->GetComponent<CSprite>();

        if (!enemyState || !enemyTrans || !enemyCol || !enemySprite) continue;
        if (enemyState->GetState() == EntityState::Dying) continue;

        const sf::FloatRect enemyBodyAABB = enemyCol->GetAABB();
        if (!IsValidRect(enemyBodyAABB)) continue;

        // Player sword hit
        if (hasValidPlayerSwordBox &&
            HasIntersection(playerSwordBox, enemyBodyAABB) &&
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
            const int enemyAttackDamage = enemyState->GetAttackDamage();
            if (enemyAttackDamage > 0)
            {
                const unsigned int enemyAttackInstance = enemyState->GetAttackInstance();
                const sf::FloatRect enemyAttackBox = ComputeWorldAttackAABB(enemyCol, enemySprite);

                if (HasIntersection(enemyAttackBox, playerBodyAABB) &&
                    playerState->TakeDamageFromAttack(
                        enemy->GetId(),
                        enemyAttackInstance,
                        enemyAttackDamage))
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
            }

            continue;
        }

        // Passive body-contact damage
        if (!HasIntersection(enemyBodyAABB, playerBodyAABB))
            continue;

        const int touchDamage = enemyState->GetTouchDamage();
        if (touchDamage > 0)
            playerState->TakeDamage(touchDamage);
    }
}