#include <fstream>
#include <sstream>
#include "EntityBase.h"
#include "EntityManager.h"
#include "CState.h"
#include "CController.h"
#include "CSprite.h"
#include "CAIPatrol.h"
#include "EngineLog.h"

bool SortCollisions(const CollisionElement& e1, const CollisionElement& e2)
{
    return e1.m_area > e2.m_area;
}

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
    // Keep sprite position synced to transform
    if (CSprite* sprite = GetComponent<CSprite>())
    {
        if (CTransform* transform = GetComponent<CTransform>())
            sprite->GetSpriteSheet().SetSpritePosition(transform->GetPosition());
    }

    // Update every attached component once per frame
    for (auto& [type, component] : m_components)
    {
        if (component) component->Update(deltaTime);
    }
}

EntityType EntityBase::GetType() const { return m_type; }
std::string EntityBase::GetName() const { return m_name; }
unsigned int EntityBase::GetId() const { return m_id; }

void EntityBase::Load(const std::string& path)
{
    std::ifstream file{ Utils::GetWorkingDirectory() + path };
    if (!file.is_open())
    {
        EngineLog::Error("Failed loading character file: " + path);
        return;
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '|') continue;

        std::stringstream keystream{ line };
        std::string type;
        keystream >> type;

        if (type == "Name") {
            keystream >> m_name;
        }
        else if (type == "Spritesheet") {
            std::string spritePath;
            keystream >> spritePath;
            CSprite* sprite = this->GetComponent<CSprite>();
            if (sprite) { sprite->Load(spritePath); }
        }
        else if (type == "Hitpoints") {
            int maxHitPoints;
            keystream >> maxHitPoints;
            CState* state = this->GetComponent<CState>();
            if (state) { state->SetHitPoints(maxHitPoints); }
        }
        else if (type == "BoundingBox") {
            float x, y;
            keystream >> x >> y;

            // Set the visual/logical size in Transform
            CTransform* transform = this->GetComponent<CTransform>();
            if (transform) { transform->SetSize(x, y); }

            // Set the physical hitbox size in Collider
            CBoxCollider* collider = this->GetComponent<CBoxCollider>();
            if (collider) {
                // If you don't have SetSize in CBoxCollider, we use the AABB directly
                collider->SetAABB(sf::FloatRect({ 0.0f, 0.0f }, { x, y }));
            }
        }
        else if (type == "DamageBox") {
            float width, height, offsetX, offsetY;
            keystream >> offsetX >> offsetY >> width >> height;
            CBoxCollider* collider = this->GetComponent<CBoxCollider>();
            if (collider) {
                collider->SetAttackAABB(sf::FloatRect({ 0.0f, 0.0f }, { width, height }));
                collider->SetAttackAABBOffset(sf::Vector2f(offsetX, offsetY));
            }
        }
        else if (type == "Speed") {
            float x, y;
            keystream >> x >> y;
            CTransform* transform = this->GetComponent<CTransform>();
            if (transform) { transform->SetSpeed(x, y); }
        }
        else if (type == "MaxVelocity") {
            float x, y;
            keystream >> x >> y;
            CTransform* transform = this->GetComponent<CTransform>();
            if (transform) { transform->SetMaxVelocity(x, y); }
        }
        else if (type == "JumpVelocity") {
            float jv;
            keystream >> jv;
            CController* controller = this->GetComponent<CController>();
            if (controller) { controller->m_jumpVelocity = jv; }
        }
        else if (type == "RangedCooldown") {
            float value;
            keystream >> value;
            CController* controller = this->GetComponent<CController>();
            if (controller && value >= 0.0f) { controller->m_rangedCooldown = value; }
        }
        else if (type == "RangedSpeed") {
            float speed;
            keystream >> speed;
            CController* controller = this->GetComponent<CController>();
            if (controller && speed > 0.0f) { controller->m_rangedSpeed = speed; }
        }
        else if (type == "RangedLifetime") {
            float lifetime;
            keystream >> lifetime;
            CController* controller = this->GetComponent<CController>();
            if (controller && lifetime > 0.0f) { controller->m_rangedLifetime = lifetime; }
        }
        else if (type == "RangedDamage") {
            int damage;
            keystream >> damage;
            CController* controller = this->GetComponent<CController>();
            if (controller && damage > 0) { controller->m_rangedDamage = damage; }
        }
        else if (type == "CoyoteTime") {
            float value;
            keystream >> value;
            CController* controller = this->GetComponent<CController>();
            if (controller && value >= 0.0f) { controller->m_coyoteTimeWindow = value; }
        }
        else if (type == "JumpBufferTime") {
            float value;
            keystream >> value;
            CController* controller = this->GetComponent<CController>();
            if (controller && value >= 0.0f) { controller->m_jumpBufferWindow = value; }
        }
        else if (type == "AttackCooldown") {
            float value;
            keystream >> value;
            CController* controller = this->GetComponent<CController>();
            if (controller && value >= 0.0f) { controller->m_attackCooldown = value; }
        }
        else if (type == "AttackDamage") {
            int damage;
            keystream >> damage;
            CState* state = this->GetComponent<CState>();
            if (state) { state->SetAttackDamage(damage); }
        }
        else if (type == "AttackKnockback") {
            float x, y;
            if (keystream >> x >> y)
            {
                // Load per-character attack knockback tuning from .char data.
                CState* state = this->GetComponent<CState>();
                if (state) { state->SetAttackKnockback(x, y); }
            }
        }
        else if (type == "TouchDamage") {
            int damage;
            keystream >> damage;
            CState* state = this->GetComponent<CState>();
            if (state) { state->SetTouchDamage(damage); }
        }
        else if (type == "InvulnerabilityTime") {
            float seconds;
            keystream >> seconds;
            CState* state = this->GetComponent<CState>();
            if (state) { state->SetInvulnerabilityTime(seconds); }
        }
        else if (type == "JumpCancelMultiplier") {
            float value;
            keystream >> value;
            CController* controller = this->GetComponent<CController>();
            if (controller && value > 0.0f && value <= 1.0f) { controller->m_jumpCancelMultiplier = value; }
        }
        else if (type == "VerticalAirThreshold") {
            float value;
            keystream >> value;
            CController* controller = this->GetComponent<CController>();
            if (controller && value >= 0.0f) { controller->m_verticalAirThreshold = value; }
        }
        else if (type == "HorizontalWalkThreshold") {
            float value;
            keystream >> value;
            CController* controller = this->GetComponent<CController>();
            if (controller && value >= 0.0f) { controller->m_horizontalWalkThreshold = value; }
        }
        else if (type == "AI_ChaseRange") {
            float value;
            keystream >> value;
            CAIPatrol* ai = this->GetComponent<CAIPatrol>();
            if (ai && value > 0.0f) { ai->m_chaseRange = value; }
        }
        else if (type == "AI_ChaseDeadZone") {
            float value;
            keystream >> value;
            CAIPatrol* ai = this->GetComponent<CAIPatrol>();
            if (ai && value >= 0.0f) { ai->m_chaseDeadZone = value; }
        }
        else if (type == "AI_ArrivalThreshold") {
            float value;
            keystream >> value;
            CAIPatrol* ai = this->GetComponent<CAIPatrol>();
            if (ai && value >= 0.0f) { ai->m_arrivalThreshold = value; }
        }
        else if (type == "AI_IdleInterval") {
            float value;
            keystream >> value;
            CAIPatrol* ai = this->GetComponent<CAIPatrol>();
            if (ai && value > 0.0f) { ai->m_idleInterval = value; }
        }
        else if (type == "AI_PatrolMinDistance") {
            int value;
            keystream >> value;
            CAIPatrol* ai = this->GetComponent<CAIPatrol>();
            if (ai && value > 0) { ai->m_patrolMinDistance = value; }
        }
        else if (type == "AI_PatrolMaxDistance") {
            int value;
            keystream >> value;
            CAIPatrol* ai = this->GetComponent<CAIPatrol>();
            if (ai && value > 0) { ai->m_patrolMaxDistance = value; }
        }
        else if (type == "AI_PatrolDirectionChance") {
            float value;
            keystream >> value;
            CAIPatrol* ai = this->GetComponent<CAIPatrol>();
            if (ai) {
                if (value < 0.0f) value = 0.0f;
                else if (value > 1.0f) value = 1.0f;
                ai->m_patrolDirectionChance = value;
            }
        }
        else if (type == "AI_AttackRange") {
            float value;
            keystream >> value;
            CAIPatrol* ai = this->GetComponent<CAIPatrol>();
            if (ai && value > 0.0f) {
                ai->m_attackRangeX = value;
                ai->m_attackRangeY = value;
            }
        }
        else if (type == "AI_AttackRangeX") {
            float value;
            keystream >> value;
            CAIPatrol* ai = this->GetComponent<CAIPatrol>();
            if (ai && value > 0.0f) { ai->m_attackRangeX = value; }
        }
        else if (type == "AI_AttackRangeY") {
            float value;
            keystream >> value;
            CAIPatrol* ai = this->GetComponent<CAIPatrol>();
            if (ai && value > 0.0f) { ai->m_attackRangeY = value; }
        }
        else
            EngineLog::WarnOnce("char.unknown_type", "Unknown type in character file: " + type);
    }

    // Set default animation to ensure the character is visible
    CSprite* sprite = this->GetComponent<CSprite>();
    if (sprite) {
        sprite->GetSpriteSheet().SetAnimation("Idle", true, true);
    }
}

void EntityBase::Destroy()
{
    m_entityManager.Remove(m_id);
}