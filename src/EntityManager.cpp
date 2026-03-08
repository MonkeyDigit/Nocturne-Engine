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
#include "CAIPatrol.h"
#include "CProjectile.h"
#include "EngineLog.h"

#ifndef NOCTURNE_DEBUG_ENTITY_LOGS
#define NOCTURNE_DEBUG_ENTITY_LOGS 0
#endif

EntityManager::EntityManager(SharedContext& context, unsigned int maxEntities)
    : m_context(context),
    m_maxEntities(maxEntities),
    m_idCounter(0)
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

    // Create a raw, empty entity directly
    std::unique_ptr<EntityBase> entity = std::make_unique<EntityBase>(*this);

    entity->m_id = m_idCounter;
    entity->SetType(type);

    if (!name.empty()) entity->m_name = name;

    // Attach the fundamental components (The "Body")
    entity->AddComponent<CTransform>();
    entity->AddComponent<CSprite>(m_context.m_textureManager);
    entity->AddComponent<CBoxCollider>();
    entity->AddComponent<CState>();

    // Attach specific components and load data
    if (type == EntityType::Player)
    {
        // Player needs a Joypad
        entity->AddComponent<CController>();

        // Load player data directly from EntityBase
        entity->Load("media/characters/Player.char");
    }
    else if (type == EntityType::Enemy)
    {
        // Enemy needs a Joypad and an AI Brain
        entity->AddComponent<CController>();
        entity->AddComponent<CAIPatrol>();

        auto typeItr = m_enemyTypes.find(name);
        if (typeItr != m_enemyTypes.end())
            entity->Load(typeItr->second);
    }

    // Store the entity in the map
    m_entities.emplace(m_idCounter, std::move(entity));
    m_idCounter++;

    return m_idCounter - 1;
}

EntityBase* EntityManager::Find(unsigned int id)
{
    auto itr = m_entities.find(id);
    if (itr == m_entities.end()) return nullptr;

    return itr->second.get(); // .get() returns the raw observer pointer
}

EntityBase* EntityManager::Find(const std::string& name)
{
    for (auto& pair : m_entities)
    {
        if (pair.second->GetName() == name)
        {
            return pair.second.get();
        }
    }

    return nullptr;
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
    m_aiSystem.Update(*this, deltaTime);
    m_controlSystem.Update(deltaTime);
    m_physicsSystem.Update(*this, m_context.m_gameMap, deltaTime);
    m_combatSystem.Update(*this);
    m_animationSystem.Update(*this, deltaTime);

    // Update each entity
    for (auto& pair : m_entities)
    {
        pair.second->Update(deltaTime);
    }

    // Camera updates last, after all movements are resolved
    m_cameraSystem.Update(*this, *m_context.m_gameMap);

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
    transform->SetSize(16.0f, 16.0f); // Default size of the fireball

    CSprite* sprite = entity->AddComponent<CSprite>(m_context.m_textureManager);
    sprite->Load("media/spritesheets/Player.sheet");

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

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '|') continue;

        std::stringstream keystream(line);
        std::string name;
        std::string charFile;

        keystream >> name >> charFile;
        m_enemyTypes.emplace(name, charFile);
    }
    file.close();
}