#include <fstream>
#include <sstream>
#include "EntityManager.h"
#include "SharedContext.h"
#include "Utilities.h"
#include "CSprite.h" 
#include "Window.h"
#include "CState.h"
#include "CController.h"
#include "CAIPatrol.h"

EntityManager::EntityManager(SharedContext& context, unsigned int maxEntities)
    : m_context(context),
    m_maxEntities(maxEntities),
    m_idCounter(0)
{
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
    m_entitiesToRemove.emplace_back(id);
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

    ProcessRemovals();
}

void EntityManager::Draw()
{
    sf::RenderWindow& window = m_context.m_window.GetRenderWindow();
    m_renderSystem.Render(*this, window);
}

void EntityManager::Purge()
{
    // unique_ptr automatically deletes the memory of all entities!
    m_entities.clear();
    m_idCounter = 0;
}

SharedContext& EntityManager::GetContext()
{
    return m_context;
}

void EntityManager::ProcessRemovals()
{
    while (!m_entitiesToRemove.empty())
    {
        unsigned int id = m_entitiesToRemove.back();
        auto itr = m_entities.find(id);

        if (itr != m_entities.end())
        {
            std::cout << "Discarding entity ID: " << id << '\n';
            m_entities.erase(itr); // Memory is freed automatically here!
        }

        m_entitiesToRemove.pop_back();
    }
}

void EntityManager::LoadEnemyTypes(const std::string& path)
{
    std::ifstream file{ Utils::GetWorkingDirectory() + path };
    if (!file.is_open())
    {
        std::cerr << "! Failed loading enemy type file: " << path << '\n';
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