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
    for (auto& pair : m_entities)
    {
        pair.second->Update(deltaTime);
    }

    EntityCollisionCheck();

    ProcessRemovals();
}

void EntityManager::Draw()
{
    sf::RenderWindow& window = m_context.m_window.GetRenderWindow();
    sf::FloatRect viewSpace = m_context.m_window.GetViewSpace();

    for (auto& pair : m_entities)
    {
        // SFML 3 uses findIntersection instead of intersects!
        if (!viewSpace.findIntersection(pair.second->m_collider->GetAABB())) continue;

        pair.second->Draw(window);
    }
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

void EntityManager::EntityCollisionCheck()
{
    if (m_entities.empty()) return;

    for (auto itr = m_entities.begin(); std::next(itr) != m_entities.end(); ++itr)
    {
        for (auto itr2 = std::next(itr); itr2 != m_entities.end(); ++itr2)
        {
            if (itr->first == itr2->first) continue;

            EntityBase* e1 = itr->second.get();
            EntityBase* e2 = itr2->second.get();

            // SFML 3: findIntersection instead of intersects
            if (e1->m_collider->GetAABB().findIntersection(e2->m_collider->GetAABB()))
            {
                e1->OnEntityCollision(*e2, false);
                e2->OnEntityCollision(*e1, false);
            }

            EntityType t1 = e1->GetType();
            EntityType t2 = e2->GetType();

            if (t1 == EntityType::Player || t1 == EntityType::Enemy)
            {
                auto* c1 = static_cast<Character*>(e1);
                if (c1->m_attackAABB.findIntersection(e2->m_collider->GetAABB()))
                {
                    c1->OnEntityCollision(*e2, true);
                }
            }

            if (t2 == EntityType::Player || t2 == EntityType::Enemy)
            {
                auto* c2 = static_cast<Character*>(e2);
                if (c2->m_attackAABB.findIntersection(e1->m_collider->GetAABB()))
                {
                    c2->OnEntityCollision(*e1, true);
                }
            }
        }
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