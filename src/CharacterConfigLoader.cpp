#include <fstream>
#include <sstream>
#include <string>
#include <initializer_list>
#include <unordered_set>
#include <vector>

#include "EntityBase.h"
#include "CSprite.h"
#include "CState.h"
#include "CTransform.h"
#include "CBoxCollider.h"
#include "CController.h"
#include "CAIPatrol.h"
#include "CharacterConfigParser.h"
#include "EngineLog.h"
#include "Utilities.h"
#include "ConfigParseUtils.h"

namespace
{
    using ParsedKeySet = std::unordered_set<std::string>;

    std::string JoinKeyList(const std::vector<std::string>& keys)
    {
        std::string joined;
        for (std::size_t i = 0; i < keys.size(); ++i)
        {
            if (i > 0) joined += ", ";
            joined += keys[i];
        }
        return joined;
    }

    void AppendMissingKeys(
        const ParsedKeySet& parsedKeys,
        std::initializer_list<const char*> requiredKeys,
        std::vector<std::string>& outMissing)
    {
        for (const char* key : requiredKeys)
        {
            if (parsedKeys.find(key) == parsedKeys.end())
                outMissing.emplace_back(key);
        }
    }

    bool ValidateRequiredKeys(
        EntityType entityType,
        const std::string& path,
        const ParsedKeySet& parsedKeys)
    {
        std::vector<std::string> missingKeys;

        // Required for every gameplay character.
        const std::initializer_list<const char*> kCommonRequiredKeys = {
            "Name",
            "Spritesheet",
            "Hitpoints",
            "BoundingBox",
            "DamageBox",
            "Speed",
            "MaxVelocity",
            "AttackDamage",
            "AttackKnockback",
            "TouchDamage",
            "InvulnerabilityTime"
        };
        AppendMissingKeys(parsedKeys, kCommonRequiredKeys, missingKeys);

        if (entityType == EntityType::Player)
        {
            const std::initializer_list<const char*> kPlayerRequiredKeys = {
                "JumpVelocity",
                "CoyoteTime",
                "JumpBufferTime",
                "AttackCooldown",
                "JumpCancelMultiplier",
                "VerticalAirThreshold",
                "HorizontalWalkThreshold",
                "RangedEnabled"
            };
            AppendMissingKeys(parsedKeys, kPlayerRequiredKeys, missingKeys);
        }
        else if (entityType == EntityType::Enemy)
        {
            const std::initializer_list<const char*> kEnemyRequiredKeys = {
                "JumpVelocity",
                "AttackCooldown",
                "AI_ChaseRange",
                "AI_ChaseDeadZone",
                "AI_ArrivalThreshold",
                "AI_IdleInterval",
                "AI_PatrolMinDistance",
                "AI_PatrolMaxDistance",
                "AI_PatrolDirectionChance",
                "AI_AttackRangeX",
                "AI_AttackRangeY"
            };
            AppendMissingKeys(parsedKeys, kEnemyRequiredKeys, missingKeys);
        }

        if (missingKeys.empty())
            return true;

        EngineLog::Error(
            "Character load failed for '" + path +
            "': missing required keys: " + JoinKeyList(missingKeys));
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

    std::unordered_set<std::string> parsedValidKeys;

    while (std::getline(file, line))
    {
        ++lineNumber;

        if (!ParseUtils::PrepareConfigLine(line)) continue;

        std::stringstream keystream{ line };
        std::string type;
        if (!(keystream >> type)) continue;

        const CharacterConfigParser::ParseContext context{
            path,
            lineNumber,
            m_name,
            spriteComp,
            stateComp,
            transformComp,
            colliderComp,
            controllerComp,
            aiComp,
            &parsedValidKeys
        };


        if (CharacterConfigParser::HandleCoreKey(type, keystream, context) ||
            CharacterConfigParser::HandleStateCombatKey(type, keystream, context) ||
            CharacterConfigParser::HandleControllerKey(type, keystream, context) ||
            CharacterConfigParser::HandleAiKey(type, keystream, context))
        {
            continue;
        }

        EngineLog::WarnOnce(
            "char.unknown_type." + path + "." + type,
            "Unknown type '" + type + "' in character file '" + path +
            "' at line " + std::to_string(lineNumber));
    }

    if (!ValidateRequiredKeys(m_type, path, parsedValidKeys))
        return false;

    // Require a valid playable Idle animation
    // Failing fast here avoids spawning invisible/broken entities
    if (spriteComp)
    {
        SpriteSheet& sheet = spriteComp->GetSpriteSheet();

        if (!sheet.HasAnimation("Idle") || !sheet.SetAnimation("Idle", true, true))
        {
            EngineLog::Error(
                "Character load failed for '" + path +
                "': missing valid 'Idle' animation in spritesheet.");
            return false;
        }
    }

    return true;
}