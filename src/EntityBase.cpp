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

namespace
{
    template <typename... Args>
    bool TryReadExact(std::stringstream& stream, Args&... args)
    {
        if (!((stream >> args) && ...))
            return false;

        std::string trailing;
        return !(stream >> trailing); // Reject extra unexpected tokens
    }

    void WarnInvalidCharValue(
        const std::string& path,
        unsigned int lineNumber,
        const std::string& key,
        const std::string& expected)
    {
        EngineLog::Warn(
            "Invalid '" + key + "' in '" + path + "' at line " +
            std::to_string(lineNumber) + " (expected: " + expected + ")");
    }
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
    unsigned int lineNumber = 0;

    while (std::getline(file, line))
    {
        ++lineNumber;

        // Support inline comments and full-line comments
        const size_t commentPos = line.find('#');
        if (commentPos != std::string::npos)
            line.erase(commentPos);

        // Skip empty/whitespace lines and custom comment marker
        const size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) continue;
        if (line[first] == '|') continue;

        std::stringstream keystream{ line };
        std::string type;
        if (!(keystream >> type)) continue;

        if (type == "Name")
        {
            std::string name;
            if (!TryReadExact(keystream, name))
            {
                WarnInvalidCharValue(path, lineNumber, type, "Name <string>");
                continue;
            }
            m_name = name;
        }
        else if (type == "Spritesheet")
        {
            std::string spritePath;
            if (!TryReadExact(keystream, spritePath))
            {
                WarnInvalidCharValue(path, lineNumber, type, "Spritesheet <path>");
                continue;
            }

            CSprite* sprite = this->GetComponent<CSprite>();
            if (sprite) { sprite->Load(spritePath); }
        }
        else if (type == "Hitpoints")
        {
            int maxHitPoints = 0;
            if (!TryReadExact(keystream, maxHitPoints) || maxHitPoints <= 0)
            {
                WarnInvalidCharValue(path, lineNumber, type, "Hitpoints <int > 0>");
                continue;
            }

            CState* state = this->GetComponent<CState>();
            if (state) { state->SetHitPoints(maxHitPoints); }
        }
        else if (type == "BoundingBox")
        {
            float x = 0.0f, y = 0.0f;
            if (!TryReadExact(keystream, x, y) || x <= 0.0f || y <= 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "BoundingBox <width > 0> <height > 0>");
                continue;
            }

            CTransform* transform = this->GetComponent<CTransform>();
            if (transform) { transform->SetSize(x, y); }

            CBoxCollider* collider = this->GetComponent<CBoxCollider>();
            if (collider)
                collider->SetAABB(sf::FloatRect({ 0.0f, 0.0f }, { x, y }));
        }
        else if (type == "DamageBox")
        {
            float offsetX = 0.0f, offsetY = 0.0f, width = 0.0f, height = 0.0f;
            if (!TryReadExact(keystream, offsetX, offsetY, width, height) ||
                width <= 0.0f || height <= 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "DamageBox <offsetX> <offsetY> <width > 0> <height > 0>");
                continue;
            }

            CBoxCollider* collider = this->GetComponent<CBoxCollider>();
            if (collider)
            {
                collider->SetAttackAABB(sf::FloatRect({ 0.0f, 0.0f }, { width, height }));
                collider->SetAttackAABBOffset(sf::Vector2f(offsetX, offsetY));
            }
        }
        else if (type == "Speed")
        {
            float x = 0.0f, y = 0.0f;
            if (!TryReadExact(keystream, x, y) || x <= 0.0f || y <= 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "Speed <x > 0> <y > 0>");
                continue;
            }

            CTransform* transform = this->GetComponent<CTransform>();
            if (transform) { transform->SetSpeed(x, y); }
        }
        else if (type == "MaxVelocity")
        {
            float x = 0.0f, y = 0.0f;
            if (!TryReadExact(keystream, x, y) || x <= 0.0f || y <= 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "MaxVelocity <x > 0> <y > 0>");
                continue;
            }

            CTransform* transform = this->GetComponent<CTransform>();
            if (transform) { transform->SetMaxVelocity(x, y); }
        }
        else if (type == "JumpVelocity")
        {
            float jv = 0.0f;
            if (!TryReadExact(keystream, jv) || jv <= 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "JumpVelocity <float > 0>");
                continue;
            }

            CController* controller = this->GetComponent<CController>();
            if (controller) { controller->m_jumpVelocity = jv; }
        }
        else if (type == "RangedCooldown")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value) || value < 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "RangedCooldown <float >= 0>");
                continue;
            }

            CController* controller = this->GetComponent<CController>();
            if (controller) { controller->m_rangedCooldown = value; }
        }
        else if (type == "RangedSpeed")
        {
            float speed = 0.0f;
            if (!TryReadExact(keystream, speed) || speed <= 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "RangedSpeed <float > 0>");
                continue;
            }

            CController* controller = this->GetComponent<CController>();
            if (controller) { controller->m_rangedSpeed = speed; }
        }
        else if (type == "RangedLifetime")
        {
            float lifetime = 0.0f;
            if (!TryReadExact(keystream, lifetime) || lifetime <= 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "RangedLifetime <float > 0>");
                continue;
            }

            CController* controller = this->GetComponent<CController>();
            if (controller) { controller->m_rangedLifetime = lifetime; }
        }
        else if (type == "RangedDamage")
        {
            int damage = 0;
            if (!TryReadExact(keystream, damage) || damage <= 0)
            {
                WarnInvalidCharValue(path, lineNumber, type, "RangedDamage <int > 0>");
                continue;
            }

            CController* controller = this->GetComponent<CController>();
            if (controller) { controller->m_rangedDamage = damage; }
        }
        else if (type == "RangedEnabled")
        {
            int value = 0;
            if (!TryReadExact(keystream, value) || (value != 0 && value != 1))
            {
                WarnInvalidCharValue(path, lineNumber, type, "RangedEnabled <0|1>");
                continue;
            }

            CController* controller = this->GetComponent<CController>();
            if (controller) { controller->m_rangedEnabled = (value == 1); }
        }
        else if (type == "CoyoteTime")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value) || value < 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "CoyoteTime <float >= 0>");
                continue;
            }

            CController* controller = this->GetComponent<CController>();
            if (controller) { controller->m_coyoteTimeWindow = value; }
        }
        else if (type == "JumpBufferTime")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value) || value < 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "JumpBufferTime <float >= 0>");
                continue;
            }

            CController* controller = this->GetComponent<CController>();
            if (controller) { controller->m_jumpBufferWindow = value; }
        }
        else if (type == "AttackCooldown")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value) || value < 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "AttackCooldown <float >= 0>");
                continue;
            }

            CController* controller = this->GetComponent<CController>();
            if (controller) { controller->m_attackCooldown = value; }
        }
        else if (type == "AttackDamage")
        {
            int damage = 0;
            if (!TryReadExact(keystream, damage) || damage <= 0)
            {
                WarnInvalidCharValue(path, lineNumber, type, "AttackDamage <int > 0>");
                continue;
            }

            CState* state = this->GetComponent<CState>();
            if (state) { state->SetAttackDamage(damage); }
        }
        else if (type == "AttackKnockback")
        {
            float x = 0.0f, y = 0.0f;
            if (!TryReadExact(keystream, x, y) || x < 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "AttackKnockback <x >= 0> <y>");
                continue;
            }

            CState* state = this->GetComponent<CState>();
            if (state) { state->SetAttackKnockback(x, y); }
        }
        else if (type == "TouchDamage")
        {
            int damage = 0;
            if (!TryReadExact(keystream, damage) || damage < 0)
            {
                WarnInvalidCharValue(path, lineNumber, type, "TouchDamage <int >= 0>");
                continue;
            }

            CState* state = this->GetComponent<CState>();
            if (state) { state->SetTouchDamage(damage); }
        }
        else if (type == "InvulnerabilityTime")
        {
            float seconds = 0.0f;
            if (!TryReadExact(keystream, seconds) || seconds < 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "InvulnerabilityTime <float >= 0>");
                continue;
            }

            CState* state = this->GetComponent<CState>();
            if (state) { state->SetInvulnerabilityTime(seconds); }
        }
        else if (type == "JumpCancelMultiplier")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value) || value <= 0.0f || value > 1.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "JumpCancelMultiplier <0 < value <= 1>");
                continue;
            }

            CController* controller = this->GetComponent<CController>();
            if (controller) { controller->m_jumpCancelMultiplier = value; }
        }
        else if (type == "VerticalAirThreshold")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value) || value < 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "VerticalAirThreshold <float >= 0>");
                continue;
            }

            CController* controller = this->GetComponent<CController>();
            if (controller) { controller->m_verticalAirThreshold = value; }
        }
        else if (type == "HorizontalWalkThreshold")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value) || value < 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "HorizontalWalkThreshold <float >= 0>");
                continue;
            }

            CController* controller = this->GetComponent<CController>();
            if (controller) { controller->m_horizontalWalkThreshold = value; }
        }
        else if (type == "AI_ChaseRange")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value) || value <= 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "AI_ChaseRange <float > 0>");
                continue;
            }

            CAIPatrol* ai = this->GetComponent<CAIPatrol>();
            if (ai) { ai->m_chaseRange = value; }
        }
        else if (type == "AI_ChaseDeadZone")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value) || value < 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "AI_ChaseDeadZone <float >= 0>");
                continue;
            }

            CAIPatrol* ai = this->GetComponent<CAIPatrol>();
            if (ai) { ai->m_chaseDeadZone = value; }
        }
        else if (type == "AI_ArrivalThreshold")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value) || value < 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "AI_ArrivalThreshold <float >= 0>");
                continue;
            }

            CAIPatrol* ai = this->GetComponent<CAIPatrol>();
            if (ai) { ai->m_arrivalThreshold = value; }
        }
        else if (type == "AI_IdleInterval")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value) || value <= 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "AI_IdleInterval <float > 0>");
                continue;
            }

            CAIPatrol* ai = this->GetComponent<CAIPatrol>();
            if (ai) { ai->m_idleInterval = value; }
        }
        else if (type == "AI_PatrolMinDistance")
        {
            int value = 0;
            if (!TryReadExact(keystream, value) || value <= 0)
            {
                WarnInvalidCharValue(path, lineNumber, type, "AI_PatrolMinDistance <int > 0>");
                continue;
            }

            CAIPatrol* ai = this->GetComponent<CAIPatrol>();
            if (ai) { ai->m_patrolMinDistance = value; }
        }
        else if (type == "AI_PatrolMaxDistance")
        {
            int value = 0;
            if (!TryReadExact(keystream, value) || value <= 0)
            {
                WarnInvalidCharValue(path, lineNumber, type, "AI_PatrolMaxDistance <int > 0>");
                continue;
            }

            CAIPatrol* ai = this->GetComponent<CAIPatrol>();
            if (ai) { ai->m_patrolMaxDistance = value; }
        }
        else if (type == "AI_PatrolDirectionChance")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value))
            {
                WarnInvalidCharValue(path, lineNumber, type, "AI_PatrolDirectionChance <float 0..1>");
                continue;
            }

            if (value < 0.0f || value > 1.0f)
            {
                EngineLog::Warn(
                    "AI_PatrolDirectionChance out of range in '" + path + "' at line " +
                    std::to_string(lineNumber) + ". Clamping to [0,1].");
                if (value < 0.0f) value = 0.0f;
                if (value > 1.0f) value = 1.0f;
            }

            CAIPatrol* ai = this->GetComponent<CAIPatrol>();
            if (ai) { ai->m_patrolDirectionChance = value; }
        }
        else if (type == "AI_AttackRange")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value) || value <= 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "AI_AttackRange <float > 0>");
                continue;
            }

            CAIPatrol* ai = this->GetComponent<CAIPatrol>();
            if (ai)
            {
                ai->m_attackRangeX = value;
                ai->m_attackRangeY = value;
            }
        }
        else if (type == "AI_AttackRangeX")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value) || value <= 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "AI_AttackRangeX <float > 0>");
                continue;
            }

            CAIPatrol* ai = this->GetComponent<CAIPatrol>();
            if (ai) { ai->m_attackRangeX = value; }
        }
        else if (type == "AI_AttackRangeY")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value) || value <= 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "AI_AttackRangeY <float > 0>");
                continue;
            }

            CAIPatrol* ai = this->GetComponent<CAIPatrol>();
            if (ai) { ai->m_attackRangeY = value; }
        }
        else
        {
            EngineLog::WarnOnce(
                "char.unknown_type." + path + "." + type,
                "Unknown type '" + type + "' in character file '" + path +
                "' at line " + std::to_string(lineNumber));
        }
    }

    // Set default animation to ensure the character is visible
    CSprite* sprite = this->GetComponent<CSprite>();
    if (sprite && !sprite->GetSpriteSheet().SetAnimation("Idle", true, true))
    {
        EngineLog::WarnOnce(
            "char.missing_idle." + path,
            "Missing or invalid 'Idle' animation in character sheet for '" + path + "'");
    }
}

void EntityBase::Destroy()
{
    m_entityManager.Remove(m_id);
}