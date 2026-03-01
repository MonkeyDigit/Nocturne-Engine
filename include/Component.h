#pragma once
#include <SFML/Graphics.hpp>

// Forward declaration to avoid circular dependencies
class EntityBase;

class Component
{
public:
    // Every component must know which entity owns it
    Component(EntityBase* owner) : m_owner(owner) {}
    virtual ~Component() = default;

    // Virtual methods that specific components can override
    virtual void Awake() {}                        // Called immediately after the component is added
    virtual void Update(float deltaTime) {}        // Called every frame
    virtual void Draw(sf::RenderWindow& window) {} // Called during the render phase

    EntityBase* GetOwner() const { return m_owner; }

protected:
    EntityBase* m_owner;    // Pointer to the entity attached to this component
};