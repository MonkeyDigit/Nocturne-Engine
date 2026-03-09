#include "EntityManager.h"
#include "SharedContext.h"
#include "CSprite.h"
#include "CState.h"
#include "CController.h"
#include "CTransform.h"
#include "CBoxCollider.h"
#include "CAIPatrol.h"
#include "CProjectile.h"
#include "EngineLog.h"

int EntityManager::Add(EntityType type, const std::string& name)
{
    if (m_entities.size() >= m_maxEntities)
    {
        EngineLog::ErrorOnce("entity.limit.reached", "Entity limit reached");
        return -1;
    }

    std::string charPath;
    bool needsAI = false;

    if (type == EntityType::Player)
    {
        charPath = "media/characters/Player.char";
    }
    else if (type == EntityType::Enemy)
    {
        const std::string enemyTypeId = name.empty() ? "<empty>" : name;
        auto typeItr = m_enemyTypes.find(name);
        if (typeItr == m_enemyTypes.end())
        {
            EngineLog::WarnOnce(
                "enemy.type.unknown." + enemyTypeId,
                "Unknown enemy type '" + enemyTypeId + "'. Check media/lists/enemy_list.list.");
            return -1;
        }

        charPath = typeItr->second;
        needsAI = true;
    }
    else
    {
        EngineLog::WarnOnce(
            "entity.type.unsupported." + std::to_string(static_cast<int>(type)),
            "Unsupported EntityType in EntityManager::Add. Only Player and Enemy are allowed.");
        return -1;
    }

    std::unique_ptr<EntityBase> entity = std::make_unique<EntityBase>(*this);
    entity->m_id = m_idCounter;
    entity->SetType(type);

    if (!name.empty()) entity->m_name = name;

    // Base components shared by spawnable gameplay entities
    entity->AddComponent<CTransform>();
    entity->AddComponent<CSprite>(m_context.m_textureManager);
    entity->AddComponent<CBoxCollider>();
    entity->AddComponent<CState>();
    entity->AddComponent<CController>();

    if (needsAI)
        entity->AddComponent<CAIPatrol>();

    if (!entity->Load(charPath))
    {
        EngineLog::ErrorOnce(
            "entity.char.load.failed." + charPath,
            "Entity creation aborted: failed loading character file '" + charPath + "'.");
        return -1;
    }

    if (type == EntityType::Player) // Keep a canonical runtime name for player lookups and debugging
        entity->m_name = "Player";

    const unsigned int createdId = m_idCounter;
    m_entities.emplace(createdId, std::move(entity));
    ++m_idCounter;

    if (type == EntityType::Player)
        m_playerId = static_cast<int>(createdId);

    return static_cast<int>(createdId);
}

int EntityManager::SpawnProjectile(EntityBase* shooter, const sf::Vector2f& position, const sf::Vector2f& velocity, int damage, float lifespan)
{
    if (!shooter)
    {
        EngineLog::ErrorOnce("projectile.null_shooter", "SpawnProjectile called with null shooter");
        return -1;
    }

    const CController* shooterController = shooter->GetComponent<CController>();
    const GameplayTuning& tuning = m_context.m_gameplayTuning;

    const float projectileWidth =
        (shooterController && shooterController->m_rangedSizeX > 0.0f)
        ? shooterController->m_rangedSizeX
        : tuning.m_projectileFallbackWidth;

    const float projectileHeight =
        (shooterController && shooterController->m_rangedSizeY > 0.0f)
        ? shooterController->m_rangedSizeY
        : tuning.m_projectileFallbackHeight;

    const std::string projectileSheet =
        (shooterController && !shooterController->m_rangedSheetPath.empty())
        ? shooterController->m_rangedSheetPath
        : tuning.m_projectileFallbackSheet;

    const std::string projectileAnimation =
        (shooterController && !shooterController->m_rangedAnimation.empty())
        ? shooterController->m_rangedAnimation
        : tuning.m_projectileFallbackAnimation;

    if (m_entities.size() >= m_maxEntities)
    {
        EngineLog::ErrorOnce("entity.limit.reached", "Entity limit reached");
        return -1;
    }

    std::unique_ptr<EntityBase> entity = std::make_unique<EntityBase>(*this);
    entity->m_id = m_idCounter;
    entity->SetType(EntityType::Projectile);
    entity->m_name = "Projectile";

    // Transform: set starting position and flying velocity
    CTransform* transform = entity->AddComponent<CTransform>();
    transform->SetPosition(position);
    transform->SetVelocity(velocity.x, velocity.y);
    transform->SetSize(projectileWidth, projectileHeight);

    CSprite* sprite = entity->AddComponent<CSprite>(m_context.m_textureManager);
    sprite->Load(projectileSheet);

    // Force a valid animation/texture-rect for the fallback projectile visual
    SpriteSheet& projectileSheetRef = sprite->GetSpriteSheet();

    bool hasProjectileAnim = false;
    if (projectileSheetRef.HasAnimation(projectileAnimation))
    {
        hasProjectileAnim = projectileSheetRef.SetAnimation(projectileAnimation, true, true);
    }
    else if (projectileAnimation != "Idle" && projectileSheetRef.HasAnimation("Idle"))
    {
        hasProjectileAnim = projectileSheetRef.SetAnimation("Idle", true, true);
    }

    if (!hasProjectileAnim)
    {
        EngineLog::WarnOnce(
            "projectile.sprite.missing_anim." + projectileSheet + "." + projectileAnimation,
            "Projectile sheet '" + projectileSheet + "' has no usable animation '" + projectileAnimation + "'.");
    }

    sprite->SetDirection((velocity.x < 0.0f) ? Direction::Left : Direction::Right);

    // Collider: so it can hit things
    entity->AddComponent<CBoxCollider>();

    // Projectile component: defines damage, lifespan, and shooter type
    CProjectile* proj = entity->AddComponent<CProjectile>();
    proj->SetShooterType(shooter->GetType());
    proj->SetDamage(damage);
    proj->SetLifespan(lifespan);

    m_entities.emplace(m_idCounter, std::move(entity));
    m_idCounter++;

    return m_idCounter - 1;
}