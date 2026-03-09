#include <fstream>
#include <sstream>
#include <algorithm>
#include "EntityManager.h"
#include "SharedContext.h"
#include "Utilities.h"
#include "CSprite.h" 
#include "Window.h"
#include "CState.h"
#include "CController.h"
#include "CTransform.h"
#include "CBoxCollider.h"
#include "CAIPatrol.h"
#include "CProjectile.h"
#include "EngineLog.h"

#ifndef NOCTURNE_DEBUG_ENTITY_LOGS
#define NOCTURNE_DEBUG_ENTITY_LOGS 0
#endif

EntityManager::EntityManager(SharedContext& context, unsigned int maxEntities)
    : m_context(context),
    m_maxEntities(maxEntities),
    m_idCounter(0),
    m_playerId(-1)
{
    m_cameraSystem.SetInitialView(m_context.m_window.GetGameView());
    m_controlSystem.Initialize(this);
    LoadEnemyTypes("media/lists/enemy_list.list");
}

EntityManager::~EntityManager()
{
    Purge();
    m_controlSystem.Destroy();
}

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

EntityBase* EntityManager::Find(unsigned int id)
{
    auto itr = m_entities.find(id);
    if (itr == m_entities.end()) return nullptr;

    return itr->second.get(); // .get() returns the raw observer pointer
}

void EntityManager::Remove(unsigned int id)
{
    // Ignore invalid IDs
    if (m_entities.find(id) == m_entities.end()) return;

    // Avoid duplicate removals in the same frame
    if (std::find(m_entitiesToRemove.begin(), m_entitiesToRemove.end(), id) == m_entitiesToRemove.end())
    {
        m_entitiesToRemove.emplace_back(id);
    }
}

void EntityManager::Update(float deltaTime)
{
    Map* gameMap = m_context.m_gameMap;

    m_aiSystem.Update(*this, deltaTime);
    m_controlSystem.Update(deltaTime);

    if (gameMap)
    {
        m_physicsSystem.Update(*this, gameMap, deltaTime);
        m_combatSystem.Update(*this);
    }
    else
    {
        // Defensive guard: avoid null dereference if Update is called outside gameplay state
        EngineLog::WarnOnce(
            "entity.update.no_map",
            "EntityManager::Update called without an active map. Skipping physics/combat/camera.");
    }

    m_animationSystem.Update(*this, deltaTime);

    for (auto& pair : m_entities)
    {
        pair.second->Update(deltaTime);
    }

    if (gameMap)
    {
        m_cameraSystem.Update(*this, *gameMap);
        EngineLog::ResetOnce("entity.update.no_map");
    }

    ProcessRemovals();
}

void EntityManager::Draw()
{
    sf::RenderWindow& window = m_context.m_window.GetRenderWindow();
    m_renderSystem.Render(*this, window);
}

void EntityManager::Purge()
{
    m_entities.clear();
    m_entitiesToRemove.clear(); // Clear pending removals to avoid stale IDs
    m_idCounter = 0;
    m_playerId = -1;
}

SharedContext& EntityManager::GetContext()
{
    return m_context;
}

int EntityManager::SpawnProjectile(EntityBase* shooter, const sf::Vector2f& position, const sf::Vector2f& velocity, int damage, float lifespan)
{
    if (!shooter)
    {
        EngineLog::ErrorOnce("projectile.null_shooter", "SpawnProjectile called with null shooter");
        return -1;
    }

    const CController* shooterController = shooter->GetComponent<CController>();

    const float projectileWidth =
        (shooterController && shooterController->m_rangedSizeX > 0.0f)
        ? shooterController->m_rangedSizeX
        : 16.0f;

    const float projectileHeight =
        (shooterController && shooterController->m_rangedSizeY > 0.0f)
        ? shooterController->m_rangedSizeY
        : 16.0f;

    const std::string projectileSheet =
        (shooterController && !shooterController->m_rangedSheetPath.empty())
        ? shooterController->m_rangedSheetPath
        : "media/spritesheets/Player.sheet";

    const std::string projectileAnimation =
        (shooterController && !shooterController->m_rangedAnimation.empty())
        ? shooterController->m_rangedAnimation
        : "Idle";

    if (m_entities.size() >= m_maxEntities)
    {
        EngineLog::ErrorOnce("entity.limit.reached", "Entity limit reached");
        return -1;
    }

    std::unique_ptr<EntityBase> entity = std::make_unique<EntityBase>(*this);
    entity->m_id = m_idCounter;
    entity->SetType(EntityType::Projectile);
    entity->m_name = "Projectile";

    // Transform: Set starting position and flying velocity
    CTransform* transform = entity->AddComponent<CTransform>();
    transform->SetPosition(position);
    transform->SetVelocity(velocity.x, velocity.y);
    transform->SetSize(projectileWidth, projectileHeight);

    CSprite* sprite = entity->AddComponent<CSprite>(m_context.m_textureManager);
    sprite->Load(projectileSheet);

    // Force a valid animation/texture-rect for the fallback projectile visual
    bool hasProjectileAnim = sprite->GetSpriteSheet().SetAnimation(projectileAnimation, true, true);
    if (!hasProjectileAnim && projectileAnimation != "Idle")
        hasProjectileAnim = sprite->GetSpriteSheet().SetAnimation("Idle", true, true);

    if (!hasProjectileAnim)
    {
        EngineLog::WarnOnce(
            "projectile.sprite.missing_anim." + projectileSheet + "." + projectileAnimation,
            "Projectile sheet '" + projectileSheet + "' has no usable animation '" + projectileAnimation + "'.");
    }

    sprite->SetDirection((velocity.x < 0.0f) ? Direction::Left : Direction::Right);

    // Collider: So it can hit things
    entity->AddComponent<CBoxCollider>();

    // Projectile Brain: Defines damage, lifespan, and who shot it
    CProjectile* proj = entity->AddComponent<CProjectile>();
    proj->SetShooterType(shooter->GetType());
    proj->SetDamage(damage);
    proj->SetLifespan(lifespan);

    // Save and return
    m_entities.emplace(m_idCounter, std::move(entity));
    m_idCounter++;

    return m_idCounter - 1;
}

EntityBase* EntityManager::GetPlayer()
{
    if (m_playerId >= 0)
    {
        if (EntityBase* cached = Find(static_cast<unsigned int>(m_playerId)))
            return cached;

        // Cached id is stale.
        m_playerId = -1;
    }

    EntityBase* foundPlayer = nullptr;
    unsigned int foundId = 0u;

    for (auto& [id, entity] : m_entities)
    {
        if (!entity || entity->GetType() != EntityType::Player)
            continue;

        if (!foundPlayer)
        {
            foundPlayer = entity.get();
            foundId = id;
        }
        else
        {
            EngineLog::WarnOnce(
                "entity.player.multiple",
                "Multiple Player entities found. Using the first one.");
            break;
        }
    }

    if (foundPlayer)
    {
        m_playerId = static_cast<int>(foundId);
        return foundPlayer;
    }

    m_playerId = -1;
    return nullptr;
}

void EntityManager::ProcessRemovals()
{
    EngineLog::ResetOnce("entity.limit.reached");

    for (unsigned int id : m_entitiesToRemove)
    {
        auto itr = m_entities.find(id);
        if (itr != m_entities.end())
        {
#if NOCTURNE_DEBUG_ENTITY_LOGS
            EngineLog::Info("Discarding entity ID: " + std::to_string(id));
#endif
            if (static_cast<int>(id) == m_playerId)
                m_playerId = -1;
            m_entities.erase(itr);
        }
    }

    m_entitiesToRemove.clear();
}

void EntityManager::LoadEnemyTypes(const std::string& path)
{
    std::ifstream file{ Utils::GetWorkingDirectory() + path };
    if (!file.is_open())
    {
        EngineLog::Error("Failed loading enemy type file: " + path);
        return;
    }

    auto warnLine = [&](unsigned int lineNumber, const std::string& message)
        {
            EngineLog::Warn(path + " line " + std::to_string(lineNumber) + ": " + message);
        };

    std::string line;
    unsigned int lineNumber = 0;

    while (std::getline(file, line))
    {
        ++lineNumber;

        // Support inline comments and full-line comments
        const size_t commentPos = line.find('#');
        if (commentPos != std::string::npos)
            line.erase(commentPos);

        const size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) continue;
        if (line[first] == '|') continue;

        std::stringstream keystream(line);
        std::string enemyName;
        std::string charFile;

        if (!(keystream >> enemyName >> charFile))
        {
            warnLine(lineNumber, "Invalid entry (expected: <EnemyTypeId> <CharacterFile>).");
            continue;
        }

        std::string trailing;
        if (keystream >> trailing)
        {
            warnLine(lineNumber, "Too many tokens for enemy type '" + enemyName + "'.");
            continue;
        }

        auto [it, inserted] = m_enemyTypes.emplace(enemyName, charFile);
        if (!inserted)
        {
            warnLine(lineNumber, "Duplicate enemy type '" + enemyName + "'. Overwriting previous file.");
            it->second = charFile;
        }
    }
}