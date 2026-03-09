#include "EntityBase.h"
#include "EntityManager.h"
#include "CTransform.h"
#include "CSprite.h"
#include "CState.h"
#include "CProjectile.h"

EntityBase::EntityBase(EntityManager& entityManager)
    : m_entityManager(entityManager), m_name("BaseEntity"),
    m_type(EntityType::Base),m_id(0)
{}

EntityBase::~EntityBase() {}

// --- DELEGATED TRANSFORM METHODS (Wrappers for CTransform) ---
void EntityBase::SetPosition(float x, float y) {
    if (auto* transform = this->GetComponent<CTransform>()) transform->SetPosition(x, y);
}

void EntityBase::SetPosition(const sf::Vector2f& pos) {
    if (auto* transform = this->GetComponent<CTransform>()) transform->SetPosition(pos);
}

void EntityBase::AddPosition(float x, float y) {
    if (auto* transform = this->GetComponent<CTransform>()) transform->AddPosition(x, y);
}

const sf::Vector2f& EntityBase::GetPosition(){
    if (auto* transform = this->GetComponent<CTransform>()) return transform->GetPosition();
    static const sf::Vector2f emptyVec(0.0f, 0.0f); // Safety fallback
    return emptyVec;
}

void EntityBase::SetVelocity(float x, float y) {
    if (auto* transform = this->GetComponent<CTransform>()) transform->SetVelocity(x, y);
}

void EntityBase::AddVelocity(float x, float y) {
    if (auto* transform = this->GetComponent<CTransform>()) transform->AddVelocity(x, y);
}

const sf::Vector2f& EntityBase::GetVelocity(){
    if (auto* transform = this->GetComponent<CTransform>()) return transform->GetVelocity();
    static const sf::Vector2f emptyVec(0.0f, 0.0f);
    return emptyVec;
}

void EntityBase::SetMaxVelocity(float x, float y) {
    if (auto* transform = this->GetComponent<CTransform>()) transform->SetMaxVelocity(x, y);
}

const sf::Vector2f& EntityBase::GetMaxVelocity(){
    if (auto* transform = this->GetComponent<CTransform>()) return transform->GetMaxVelocity();
    static const sf::Vector2f emptyVec(0.0f, 0.0f);
    return emptyVec;
}

void EntityBase::SetAcceleration(float x, float y) {
    if (auto* transform = this->GetComponent<CTransform>()) transform->SetAcceleration(x, y);
}

void EntityBase::AddAcceleration(float x, float y) {
    if (auto* transform = this->GetComponent<CTransform>()) transform->AddAcceleration(x, y);
}

const sf::Vector2f& EntityBase::GetAcceleration() {
    if (auto* transform = this->GetComponent<CTransform>()) return transform->GetAcceleration();
    static const sf::Vector2f emptyVec(0.0f, 0.0f);
    return emptyVec;
}

void EntityBase::SetSpeed(float x, float y) {
    if (auto* transform = this->GetComponent<CTransform>()) transform->SetSpeed(x, y);
}

const sf::Vector2f& EntityBase::GetSpeed() {
    if (auto* transform = this->GetComponent<CTransform>()) return transform->GetSpeed();
    static const sf::Vector2f emptyVec(0.0f, 0.0f);
    return emptyVec;
}

void EntityBase::SetFriction(float x, float y) {
    if (auto* transform = this->GetComponent<CTransform>()) transform->SetFriction(x, y);
}

const sf::Vector2f& EntityBase::GetFriction() {
    if (auto* transform = this->GetComponent<CTransform>()) return transform->GetFriction();
    static const sf::Vector2f emptyVec(0.0f, 0.0f);
    return emptyVec;
}

void EntityBase::SetSize(float x, float y) {
    if (auto* transform = this->GetComponent<CTransform>()) transform->SetSize(x, y);
}

const sf::Vector2f& EntityBase::GetSize() {
    if (auto* transform = this->GetComponent<CTransform>()) return transform->GetSize();
    static const sf::Vector2f emptyVec(0.0f, 0.0f);
    return emptyVec;
}

void EntityBase::Update(float deltaTime)
{
    CTransform* transform = GetComponent<CTransform>();
    CSprite* sprite = GetComponent<CSprite>();

    // Keep sprite transform synced before animation frame advance
    if (sprite && transform)
        sprite->GetSpriteSheet().SetSpritePosition(transform->GetPosition());

    // Fixed update order for timing-sensitive components
    if (CState* state = GetComponent<CState>())
        state->Update(deltaTime);

    if (CProjectile* projectile = GetComponent<CProjectile>())
        projectile->Update(deltaTime);

    if (sprite)
        sprite->Update(deltaTime);

    // Update any remaining custom components exactly once
    for (auto& [type, component] : m_components)
    {
        if (!component) continue;

        if (type == typeid(CState) ||
            type == typeid(CProjectile) ||
            type == typeid(CSprite))
        {
            continue;
        }

        component->Update(deltaTime);
    }
}

EntityType EntityBase::GetType() const { return m_type; }
std::string EntityBase::GetName() const { return m_name; }
unsigned int EntityBase::GetId() const { return m_id; }

void EntityBase::Destroy()
{
    m_entityManager.Remove(m_id);
}

void EntityBase::DestroyAndDisableProjectileDamage()
{
    if (m_type == EntityType::Projectile)
    {
        // Remove projectile payload first so other systems ignore it in this frame
        RemoveComponent<CProjectile>();
    }

    Destroy();
}