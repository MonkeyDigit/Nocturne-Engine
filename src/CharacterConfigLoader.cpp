#include <fstream>
#include <sstream>
#include <string>

#include "EntityBase.h"
#include "CState.h"
#include "CController.h"
#include "CSprite.h"
#include "CTransform.h"
#include "CBoxCollider.h"
#include "CAIPatrol.h"
#include "EngineLog.h"
#include "Utilities.h"
#include "ConfigParseUtils.h"

namespace
{
    using ParseUtils::TryReadExact;

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

    void WarnMissingComponentForKey(
        const std::string& path,
        unsigned int lineNumber,
        const std::string& key,
        const std::string& componentName)
    {
        EngineLog::WarnOnce(
            "char.missing_component." + path + "." + key + "." + componentName,
            "Key '" + key + "' in character file '" + path + "' at line " +
            std::to_string(lineNumber) + " requires component '" + componentName +
            "', but the entity does not have it.");
    }

    bool HandleCoreConfigKey(
        const std::string& type,
        std::stringstream& keystream,
        const std::string& path,
        unsigned int lineNumber,
        std::string& entityName,
        CSprite* sprite,
        CState* state,
        CTransform* transform,
        CBoxCollider* collider)
    {
        if (type == "Name")
        {
            std::string name;
            if (!TryReadExact(keystream, name))
            {
                WarnInvalidCharValue(path, lineNumber, type, "Name <string>");
                return true;
            }

            entityName = name;
            return true;
        }

        if (type == "Spritesheet")
        {
            std::string spritePath;
            if (!TryReadExact(keystream, spritePath))
            {
                WarnInvalidCharValue(path, lineNumber, type, "Spritesheet <path>");
                return true;
            }

            if (!sprite)
            {
                WarnMissingComponentForKey(path, lineNumber, type, "CSprite");
                return true;
            }
            sprite->Load(spritePath);

            return true;
        }

        if (type == "Hitpoints")
        {
            int maxHitPoints = 0;
            if (!TryReadExact(keystream, maxHitPoints) || maxHitPoints <= 0)
            {
                WarnInvalidCharValue(path, lineNumber, type, "Hitpoints <int > 0>");
                return true;
            }

            if (!state)
            {
                WarnMissingComponentForKey(path, lineNumber, type, "CState");
                return true;
            }
            state->SetHitPoints(maxHitPoints);

            return true;
        }

        if (type == "BoundingBox")
        {
            float x = 0.0f, y = 0.0f;
            if (!TryReadExact(keystream, x, y) || x <= 0.0f || y <= 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "BoundingBox <width > 0> <height > 0>");
                return true;
            }

            if (!transform)
                WarnMissingComponentForKey(path, lineNumber, type, "CTransform");
            else
                transform->SetSize(x, y);

            if (!collider)
                WarnMissingComponentForKey(path, lineNumber, type, "CBoxCollider");
            else
                collider->SetAABB(sf::FloatRect({ 0.0f, 0.0f }, { x, y }));

            return true;
        }

        if (type == "DamageBox")
        {
            float offsetX = 0.0f, offsetY = 0.0f, width = 0.0f, height = 0.0f;
            if (!TryReadExact(keystream, offsetX, offsetY, width, height) ||
                width <= 0.0f || height <= 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "DamageBox <offsetX> <offsetY> <width > 0> <height > 0>");
                return true;
            }

            if (!collider)
            {
                WarnMissingComponentForKey(path, lineNumber, type, "CBoxCollider");
                return true;
            }

            collider->SetAttackAABB(sf::FloatRect({ 0.0f, 0.0f }, { width, height }));
            collider->SetAttackAABBOffset(sf::Vector2f(offsetX, offsetY));

            return true;
        }

        if (type == "Speed")
        {
            float x = 0.0f, y = 0.0f;
            if (!TryReadExact(keystream, x, y) || x <= 0.0f || y <= 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "Speed <x > 0> <y > 0>");
                return true;
            }

            if (!transform)
            {
                WarnMissingComponentForKey(path, lineNumber, type, "CTransform");
                return true;
            }

            transform->SetSpeed(x, y);

            return true;
        }

        if (type == "MaxVelocity")
        {
            float x = 0.0f, y = 0.0f;
            if (!TryReadExact(keystream, x, y) || x <= 0.0f || y <= 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "MaxVelocity <x > 0> <y > 0>");
                return true;
            }

            if (!transform)
            {
                WarnMissingComponentForKey(path, lineNumber, type, "CTransform");
                return true;
            }

            transform->SetMaxVelocity(x, y);
            return true;
        }

        return false;
    }

    bool HandleStateCombatConfigKey(
        CState* state,
        const std::string& type,
        std::stringstream& keystream,
        const std::string& path,
        unsigned int lineNumber)
    {
        const bool isStateCombatKey =
            (type == "AttackDamage") ||
            (type == "AttackKnockback") ||
            (type == "TouchDamage") ||
            (type == "InvulnerabilityTime");

        if (isStateCombatKey && !state)
        {
            WarnMissingComponentForKey(path, lineNumber, type, "CState");
            return true;
        }

        if (type == "AttackDamage")
        {
            int damage = 0;
            if (!TryReadExact(keystream, damage) || damage <= 0)
            {
                WarnInvalidCharValue(path, lineNumber, type, "AttackDamage <int > 0>");
                return true;
            }

            state->SetAttackDamage(damage);
            return true;
        }

        if (type == "AttackKnockback")
        {
            float x = 0.0f, y = 0.0f;
            if (!TryReadExact(keystream, x, y) || x < 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "AttackKnockback <x >= 0> <y>");
                return true;
            }

            state->SetAttackKnockback(x, y);
            return true;
        }

        if (type == "TouchDamage")
        {
            int damage = 0;
            if (!TryReadExact(keystream, damage) || damage < 0)
            {
                WarnInvalidCharValue(path, lineNumber, type, "TouchDamage <int >= 0>");
                return true;
            }

            state->SetTouchDamage(damage);
            return true;
        }

        if (type == "InvulnerabilityTime")
        {
            float seconds = 0.0f;
            if (!TryReadExact(keystream, seconds) || seconds < 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "InvulnerabilityTime <float >= 0>");
                return true;
            }

            state->SetInvulnerabilityTime(seconds);
            return true;
        }

        return false;
    }

    bool HandleControllerConfigKey(
        CController* controller,
        const std::string& type,
        std::stringstream& keystream,
        const std::string& path,
        unsigned int lineNumber)
    {
        const bool isControllerKey =
            (type == "JumpVelocity") ||
            (type == "RangedCooldown") ||
            (type == "RangedSpeed") ||
            (type == "RangedLifetime") ||
            (type == "RangedDamage") ||
            (type == "RangedSpawnOffset") ||
            (type == "RangedSize") ||
            (type == "RangedSheet") ||
            (type == "RangedAnimation") ||
            (type == "RangedEnabled") ||
            (type == "CoyoteTime") ||
            (type == "JumpBufferTime") ||
            (type == "AttackCooldown") ||
            (type == "JumpCancelMultiplier") ||
            (type == "VerticalAirThreshold") ||
            (type == "HorizontalWalkThreshold");

        if (isControllerKey && !controller)
        {
            WarnMissingComponentForKey(path, lineNumber, type, "CController");
            return true;
        }

        if (type == "JumpVelocity")
        {
            float jv = 0.0f;
            if (!TryReadExact(keystream, jv) || jv <= 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "JumpVelocity <float > 0>");
                return true;
            }

            controller->m_jumpVelocity = jv;
            return true;
        }

        if (type == "RangedCooldown")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value) || value < 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "RangedCooldown <float >= 0>");
                return true;
            }

            controller->m_rangedCooldown = value;
            return true;
        }

        if (type == "RangedSpeed")
        {
            float speed = 0.0f;
            if (!TryReadExact(keystream, speed) || speed <= 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "RangedSpeed <float > 0>");
                return true;
            }

            controller->m_rangedSpeed = speed;
            return true;
        }

        if (type == "RangedLifetime")
        {
            float lifetime = 0.0f;
            if (!TryReadExact(keystream, lifetime) || lifetime <= 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "RangedLifetime <float > 0>");
                return true;
            }

            controller->m_rangedLifetime = lifetime;
            return true;
        }

        if (type == "RangedDamage")
        {
            int damage = 0;
            if (!TryReadExact(keystream, damage) || damage <= 0)
            {
                WarnInvalidCharValue(path, lineNumber, type, "RangedDamage <int > 0>");
                return true;
            }

            controller->m_rangedDamage = damage;
            return true;
        }

        if (type == "RangedSpawnOffset")
        {
            float offsetX = 0.0f, offsetY = 0.0f;
            if (!TryReadExact(keystream, offsetX, offsetY) || offsetX < 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "RangedSpawnOffset <x >= 0> <y>");
                return true;
            }

            controller->m_rangedSpawnOffsetX = offsetX;
            controller->m_rangedSpawnOffsetY = offsetY;
            return true;
        }

        if (type == "RangedSize")
        {
            float sx = 0.0f, sy = 0.0f;
            if (!TryReadExact(keystream, sx, sy) || sx <= 0.0f || sy <= 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "RangedSize <x > 0> <y > 0>");
                return true;
            }

            controller->m_rangedSizeX = sx;
            controller->m_rangedSizeY = sy;
            return true;
        }

        if (type == "RangedSheet")
        {
            std::string sheetPath;
            if (!TryReadExact(keystream, sheetPath) || sheetPath.empty())
            {
                WarnInvalidCharValue(path, lineNumber, type, "RangedSheet <path>");
                return true;
            }

            controller->m_rangedSheetPath = sheetPath;
            return true;
        }

        if (type == "RangedAnimation")
        {
            std::string animName;
            if (!TryReadExact(keystream, animName) || animName.empty())
            {
                WarnInvalidCharValue(path, lineNumber, type, "RangedAnimation <name>");
                return true;
            }

            controller->m_rangedAnimation = animName;
            return true;
        }

        if (type == "RangedEnabled")
        {
            int value = 0;
            if (!TryReadExact(keystream, value) || (value != 0 && value != 1))
            {
                WarnInvalidCharValue(path, lineNumber, type, "RangedEnabled <0|1>");
                return true;
            }

            controller->m_rangedEnabled = (value == 1);
            return true;
        }

        if (type == "CoyoteTime")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value) || value < 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "CoyoteTime <float >= 0>");
                return true;
            }

            controller->m_coyoteTimeWindow = value;
            return true;
        }

        if (type == "JumpBufferTime")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value) || value < 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "JumpBufferTime <float >= 0>");
                return true;
            }

            controller->m_jumpBufferWindow = value;
            return true;
        }

        if (type == "AttackCooldown")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value) || value < 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "AttackCooldown <float >= 0>");
                return true;
            }

            controller->m_attackCooldown = value;
            return true;
        }

        if (type == "JumpCancelMultiplier")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value) || value <= 0.0f || value > 1.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "JumpCancelMultiplier <0 < value <= 1>");
                return true;
            }

            controller->m_jumpCancelMultiplier = value;
            return true;
        }

        if (type == "VerticalAirThreshold")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value) || value < 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "VerticalAirThreshold <float >= 0>");
                return true;
            }

            controller->m_verticalAirThreshold = value;
            return true;
        }

        if (type == "HorizontalWalkThreshold")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value) || value < 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "HorizontalWalkThreshold <float >= 0>");
                return true;
            }

            controller->m_horizontalWalkThreshold = value;
            return true;
        }

        return false;
    }

    bool HandleAiConfigKey(
        CAIPatrol* ai,
        const std::string& type,
        std::stringstream& keystream,
        const std::string& path,
        unsigned int lineNumber)
    {
        const bool isAiKey =
            (type == "AI_ChaseRange") ||
            (type == "AI_ChaseDeadZone") ||
            (type == "AI_ArrivalThreshold") ||
            (type == "AI_IdleInterval") ||
            (type == "AI_PatrolMinDistance") ||
            (type == "AI_PatrolMaxDistance") ||
            (type == "AI_PatrolDirectionChance") ||
            (type == "AI_AttackRange") ||
            (type == "AI_AttackRangeX") ||
            (type == "AI_AttackRangeY");

        if (isAiKey && !ai)
        {
            WarnMissingComponentForKey(path, lineNumber, type, "CAIPatrol");
            return true;
        }

        if (type == "AI_ChaseRange")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value) || value <= 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "AI_ChaseRange <float > 0>");
                return true;
            }

            ai->m_chaseRange = value;
            return true;
        }

        if (type == "AI_ChaseDeadZone")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value) || value < 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "AI_ChaseDeadZone <float >= 0>");
                return true;
            }

            ai->m_chaseDeadZone = value;
            return true;
        }

        if (type == "AI_ArrivalThreshold")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value) || value < 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "AI_ArrivalThreshold <float >= 0>");
                return true;
            }

            ai->m_arrivalThreshold = value;
            return true;
        }

        if (type == "AI_IdleInterval")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value) || value <= 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "AI_IdleInterval <float > 0>");
                return true;
            }

            ai->m_idleInterval = value;
            return true;
        }

        if (type == "AI_PatrolMinDistance")
        {
            int value = 0;
            if (!TryReadExact(keystream, value) || value <= 0)
            {
                WarnInvalidCharValue(path, lineNumber, type, "AI_PatrolMinDistance <int > 0>");
                return true;
            }

            ai->m_patrolMinDistance = value;
            return true;
        }

        if (type == "AI_PatrolMaxDistance")
        {
            int value = 0;
            if (!TryReadExact(keystream, value) || value <= 0)
            {
                WarnInvalidCharValue(path, lineNumber, type, "AI_PatrolMaxDistance <int > 0>");
                return true;
            }

            ai->m_patrolMaxDistance = value;
            return true;
        }

        if (type == "AI_PatrolDirectionChance")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value))
            {
                WarnInvalidCharValue(path, lineNumber, type, "AI_PatrolDirectionChance <float 0..1>");
                return true;
            }

            if (value < 0.0f || value > 1.0f)
            {
                EngineLog::Warn(
                    "AI_PatrolDirectionChance out of range in '" + path + "' at line " +
                    std::to_string(lineNumber) + ". Clamping to [0,1].");
                if (value < 0.0f) value = 0.0f;
                if (value > 1.0f) value = 1.0f;
            }

            ai->m_patrolDirectionChance = value;
            return true;
        }

        if (type == "AI_AttackRange")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value) || value <= 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "AI_AttackRange <float > 0>");
                return true;
            }

            ai->m_attackRangeX = value;
            ai->m_attackRangeY = value;
            return true;
        }

        if (type == "AI_AttackRangeX")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value) || value <= 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "AI_AttackRangeX <float > 0>");
                return true;
            }

            ai->m_attackRangeX = value;
            return true;
        }

        if (type == "AI_AttackRangeY")
        {
            float value = 0.0f;
            if (!TryReadExact(keystream, value) || value <= 0.0f)
            {
                WarnInvalidCharValue(path, lineNumber, type, "AI_AttackRangeY <float > 0>");
                return true;
            }

            ai->m_attackRangeY = value;
            return true;
        }

        return false;
    }
}

bool EntityBase::Load(const std::string& path)
{
    std::ifstream file{ Utils::GetWorkingDirectory() + path };
    if (!file.is_open())
    {
        EngineLog::Error("Failed loading character file: " + path);
        return false;
    }

    CSprite* spriteComp = this->GetComponent<CSprite>();
    CState* stateComp = this->GetComponent<CState>();
    CTransform* transformComp = this->GetComponent<CTransform>();
    CBoxCollider* colliderComp = this->GetComponent<CBoxCollider>();
    CController* controllerComp = this->GetComponent<CController>();
    CAIPatrol* aiComp = this->GetComponent<CAIPatrol>();

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

        if (HandleCoreConfigKey(
            type, keystream, path, lineNumber, m_name,
            spriteComp, stateComp, transformComp, colliderComp))
            continue;

        if (HandleStateCombatConfigKey(stateComp, type, keystream, path, lineNumber))
            continue;

        if (HandleControllerConfigKey(controllerComp, type, keystream, path, lineNumber))
            continue;

        if (HandleAiConfigKey(aiComp, type, keystream, path, lineNumber))
            continue;

        EngineLog::WarnOnce(
            "char.unknown_type." + path + "." + type,
            "Unknown type '" + type + "' in character file '" + path +
            "' at line " + std::to_string(lineNumber));

    }

    // Set default animation to ensure the character is visible
    if (spriteComp && !spriteComp->GetSpriteSheet().SetAnimation("Idle", true, true))
    {
        EngineLog::WarnOnce(
            "char.missing_idle." + path,
            "Missing or invalid 'Idle' animation in character sheet for '" + path + "'");
    }

    return true;
}