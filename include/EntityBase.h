#pragma once
#include <memory>
#include <string>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <SFML/System/Vector2.hpp>
#include "Component.h"

class EntityManager;

// Moved here to solve circular dependency issues
enum class EntityType
{
    Base = 0, Enemy, Player, Projectile
};

class EntityBase
{
    friend class EntityManager;
public:
    EntityBase(EntityManager& entityManager);
    virtual ~EntityBase();

    void SetType(EntityType type) { m_type = type; }

    // --- DELEGATED TRANSFORM METHODS ---
    void SetPosition(float x, float y);
    void SetPosition(const sf::Vector2f& pos);
    void AddPosition(float x, float y);
    const sf::Vector2f& GetPosition();

    void SetVelocity(float x, float y);
    void AddVelocity(float x, float y);
    const sf::Vector2f& GetVelocity();

    void SetMaxVelocity(float x, float y);
    const sf::Vector2f& GetMaxVelocity();

    void SetAcceleration(float x, float y);
    void AddAcceleration(float x, float y);
    const sf::Vector2f& GetAcceleration();

    void SetSpeed(float x, float y);
    const sf::Vector2f& GetSpeed();

    void SetFriction(float x, float y);
    const sf::Vector2f& GetFriction();

    void SetSize(float x, float y);
    const sf::Vector2f& GetSize();

    virtual void Update(float deltaTime);

    EntityType GetType() const;
    std::string GetName() const;
    unsigned int GetId() const;

    // ENTITY COMPONENT SYSTEM

    // Adds a component of type T to the entity
    template <typename T, typename... Args>
    T* AddComponent(Args&&... args)
    {
        // Ensure that T actually inherits from Component at compile-time
        static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");

        // Create the component, passing 'this' (the owner) and any additional arguments
        auto newComponent = std::make_unique<T>(this, std::forward<Args>(args)...);
        T* rawPtr = newComponent.get();

        // Store the component in the map using its type as the key
        m_components[typeid(T)] = std::move(newComponent);

        rawPtr->Awake(); // Initialize the component
        return rawPtr;
    }

    // Retrieves a component of type T. Returns nullptr if not found
    template <typename T>
    T* GetComponent()
    {
        auto it = m_components.find(typeid(T));
        if (it != m_components.end())
        {
            return static_cast<T*>(it->second.get());
        }
        return nullptr;
    }

    // Removes a component of type T from the entity
    template <typename T>
    void RemoveComponent()
    {
        m_components.erase(typeid(T));
    }

    bool Load(const std::string& path);
    void Destroy();

protected:

    std::string m_name;
    EntityType m_type;
    unsigned int m_id;
    EntityManager& m_entityManager;

    // Map storing the components. Uses std::type_index for fast and safe type lookups
    std::unordered_map<std::type_index, std::unique_ptr<Component>> m_components;
};