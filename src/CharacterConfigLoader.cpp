#include <fstream>
#include <sstream>
#include <string>

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
            aiComp
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

    // Ensure a visible fallback animation if present
    if (spriteComp)
    {
        SpriteSheet& sheet = spriteComp->GetSpriteSheet();

        if (!sheet.HasAnimation("Idle") || !sheet.SetAnimation("Idle", true, true))
        {
            EngineLog::WarnOnce(
                "char.missing_idle." + path,
                "Missing or invalid 'Idle' animation in character sheet for '" + path + "'");
        }
    }

    return true;
}