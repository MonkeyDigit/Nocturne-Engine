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
    if (spriteComp && !spriteComp->GetSpriteSheet().SetAnimation("Idle", true, true))
    {
        EngineLog::WarnOnce(
            "char.missing_idle." + path,
            "Missing or invalid 'Idle' animation in character sheet for '" + path + "'");
    }

    return true;
}