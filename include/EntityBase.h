#pragma once
#pragma once
#include <unordered_map>
#include <memory>
#include <typeindex>
#include "Map.h"
#include "Component.h"
#include "CTransform.h"

class EntityManager;

struct CollisionElement
{
    CollisionElement(float area, TileInfo* info, const sf::FloatRect& bounds)
        : m_area(area), m_tile(info), m_tileBounds(bounds) {
    }

    float m_area;
    TileInfo* m_tile;
    sf::FloatRect m_tileBounds;
};

using Collisions = std::vector<CollisionElement>;

bool SortCollisions(const CollisionElement& e1, const CollisionElement& e2);

enum class EntityState
{
    Idle, Walking, Jumping, Attacking, Hurt, Dying
};

// Moved here to solve circular dependency issues
enum class EntityType
{
    Base = 0, Enemy, Player
};

class EntityBase
{
    friend class EntityManager;
public:
    EntityBase(EntityManager& entityManager);
    virtual ~EntityBase();

    // --- DELEGATED TRANSFORM METHODS ---
    void SetPosition(float x, float y) { m_transform->SetPosition(x, y); }
    void SetPosition(const sf::Vector2f& pos) { m_transform->SetPosition(pos); }
    void AddPosition(float x, float y) { m_transform->AddPosition(x, y); }
    const sf::Vector2f& GetPosition() const { return m_transform->GetPosition(); }

    void SetVelocity(float x, float y) { m_transform->SetVelocity(x, y); }
    void AddVelocity(float x, float y) { m_transform->AddVelocity(x, y); }
    const sf::Vector2f& GetVelocity() const { return m_transform->GetVelocity(); }

    void SetMaxVelocity(float x, float y) { m_transform->SetMaxVelocity(x, y); }
    const sf::Vector2f& GetMaxVelocity() const { return m_transform->GetMaxVelocity(); }

    void SetAcceleration(float x, float y) { m_transform->SetAcceleration(x, y); }
    void AddAcceleration(float x, float y) { m_transform->AddAcceleration(x, y); }
    const sf::Vector2f& GetAcceleration() const { return m_transform->GetAcceleration(); }

    void SetSpeed(float x, float y) { m_transform->SetSpeed(x, y); }
    const sf::Vector2f& GetSpeed() const { return m_transform->GetSpeed(); }

    void SetFriction(float x, float y) { m_transform->SetFriction(x, y); }
    const sf::Vector2f& GetFriction() const { return m_transform->GetFriction(); }

    void SetSize(float x, float y) { m_transform->SetSize(x, y); }
    const sf::Vector2f& GetSize() const { return m_transform->GetSize(); }

    void SetState(EntityState state);
    // TODO: Togliere?
    void Move(float x, float y);
    void ApplyFriction(float x, float y);

    virtual void Update(float deltaTime);
    virtual void Draw(sf::RenderWindow& window) = 0;
    // Method for what THIS entity does TO the collider entity
    virtual void OnEntityCollision(EntityBase& collider, bool attack) = 0;

    EntityType GetType() const;
    std::string GetName() const;
    unsigned int GetId() const;
    EntityState GetState() const;

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

protected:
    void UpdateAABB();
    void CheckCollisions();
    void ResolveCollisions();

    std::string m_name;
    EntityType m_type;
    unsigned int m_id;
    TileInfo* m_referenceTile;
    sf::FloatRect m_AABB;
    EntityState m_state;
    bool m_jumping;

    bool m_collidingOnX;
    bool m_collidingOnY;
    Collisions m_collisions;
    EntityManager& m_entityManager;

    // --- COMPONENT SHORTCUTS ---
    CTransform* m_transform;

    // Map storing the components. Uses std::type_index for fast and safe type lookups
    std::unordered_map<std::type_index, std::unique_ptr<Component>> m_components;
};