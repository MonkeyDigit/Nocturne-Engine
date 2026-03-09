#include <algorithm>
#include "EntityManager.h"
#include "SharedContext.h"
#include "Window.h"
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

EntityBase* EntityManager::Find(unsigned int id)
{
    auto itr = m_entities.find(id);
    if (itr == m_entities.end()) return nullptr;

    return itr->second.get(); // .get() returns the raw observer pointer
}

void EntityManager::Remove(unsigned int id)
{
    // Ignore invalid IDs.
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