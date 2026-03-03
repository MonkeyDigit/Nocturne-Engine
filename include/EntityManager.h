#pragma once
#include <unordered_map>
#include <vector>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include "EntityBase.h"
#include "PhysicsSystem.h"
#include "RenderSystem.h"
#include "AnimationSystem.h"

struct SharedContext;

// Use unique_ptr to own the entities
// When an entity is erased from this map, its memory is automatically freed
using EntityContainer = std::unordered_map<unsigned int, std::unique_ptr<EntityBase>>;

// The factory returns a unique_ptr to hand over ownership to the EntityManager
using EntityFactory = std::unordered_map<EntityType, std::function<std::unique_ptr<EntityBase>()>>;

using EnemyTypes = std::unordered_map<std::string, std::string>;

class EntityManager
{
public:
    EntityManager(SharedContext& context, unsigned int maxEntities);
    ~EntityManager(); // No manual delete needed anymore

    int Add(EntityType type, const std::string& name = "");

    // Find returns raw pointers because it's just meant for OBSERVATION, not ownership transfer
    // If the entity is not found, it can safely return nullptr
    EntityBase* Find(unsigned int id);
    EntityBase* Find(const std::string& name);

    void Remove(unsigned int id);
    void Update(float deltaTime);
    void Draw();
    void Purge();

    SharedContext& GetContext();

    std::unordered_map<unsigned int, std::unique_ptr<EntityBase>>& GetEntities()
    { return m_entities; }

private:
    template<class T>
    void RegisterEntity(EntityType type)
    {
        m_entityFactory[type] = [this]() -> std::unique_ptr<EntityBase>
            {
                return std::make_unique<T>(*this);
            };
    }

    void ProcessRemovals();
    void LoadEnemyTypes(const std::string& path);
    void EntityCollisionCheck();

    EntityContainer m_entities;
    EnemyTypes m_enemyTypes;
    EntityFactory m_entityFactory;

    SharedContext& m_context;

    unsigned int m_idCounter;
    unsigned int m_maxEntities;
    std::vector<unsigned int> m_entitiesToRemove;

    PhysicsSystem m_physicsSystem;
    RenderSystem m_renderSystem;
    AnimationSystem m_animationSystem;
};