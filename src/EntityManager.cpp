#include <fstream>
#include <sstream>
#include "EntityManager.h"
#include "SharedContext.h"
#include "Utilities.h"
#include "Player.h"
#include "Enemy.h"
#include "Character.h" 
#include "Window.h"

EntityManager::EntityManager(SharedContext& context, unsigned int maxEntities)
    : m_context(context),
    m_maxEntities(maxEntities),
    m_idCounter(0)
{
    m_controlSystem.Initialize(this);
    LoadEnemyTypes("media/lists/enemy_list.list");

    RegisterEntity<Player>(EntityType::Player);
    RegisterEntity<Enemy>(EntityType::Enemy);
}

EntityManager::~EntityManager()
{
    Purge();
}

int EntityManager::Add(EntityType type, const std::string& name)
{
    auto factoryItr = m_entityFactory.find(type);
    if (factoryItr == m_entityFactory.end()) return -1;

    // Create the entity via factory
    std::unique_ptr<EntityBase> entity = factoryItr->second();

    entity->m_id = m_idCounter;
    if (!name.empty()) entity->m_name = name;

    if (type == EntityType::Enemy)
    {
        auto typeItr = m_enemyTypes.find(name);
        if (typeItr != m_enemyTypes.end())
        {
            // dynamic_cast is safer, but static_cast is faster
            // We use static_cast since we know it's an enemy
            auto* enemy = static_cast<Enemy*>(entity.get());
            enemy->Load(typeItr->second);
        }
    }

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
    // Calculate damage and knockbacks before moving
    m_combatSystem.Update(*this);
    m_physicsSystem.Update(*this, m_context.m_gameMap, deltaTime);
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
    // TODO: Avere window come argomento?
    sf::RenderWindow& window = m_context.m_window.GetRenderWindow();
    m_renderSystem.Render(*this, window);
}

void EntityManager::Purge()
{
    // unique_ptr automatically deletes the memory of all entities!
    m_entities.clear();
    m_idCounter = 0;

    m_controlSystem.Destroy();
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