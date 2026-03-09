#pragma once

#include <sstream>
#include <string>

class CSprite;
class CState;
class CTransform;
class CBoxCollider;
class CController;
class CAIPatrol;

namespace CharacterConfigParser
{
    struct ParseContext
    {
        const std::string& path;
        unsigned int lineNumber;
        std::string& entityName;
        CSprite* sprite;
        CState* state;
        CTransform* transform;
        CBoxCollider* collider;
        CController* controller;
        CAIPatrol* ai;
    };

    // Shared diagnostics helpers used by all parser modules
    void WarnInvalidValue(
        const std::string& path,
        unsigned int lineNumber,
        const std::string& key,
        const std::string& expected);

    void WarnMissingComponent(
        const std::string& path,
        unsigned int lineNumber,
        const std::string& key,
        const std::string& componentName);

    bool HandleCoreKey(const std::string& type, std::stringstream& keystream, const ParseContext& context);
    bool HandleStateCombatKey(const std::string& type, std::stringstream& keystream, const ParseContext& context);
    bool HandleControllerKey(const std::string& type, std::stringstream& keystream, const ParseContext& context);
    bool HandleAiKey(const std::string& type, std::stringstream& keystream, const ParseContext& context);
}