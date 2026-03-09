#include <unordered_map>

#include "CharacterConfigParser.h"
#include "CAIPatrol.h"
#include "EngineLog.h"
#include "ConfigParseUtils.h"

namespace
{
    using ParseUtils::TryReadExact;
    using CfgCtx = CharacterConfigParser::ParseContext;
    using HandlerFn = bool (*)(std::stringstream&, const CfgCtx&, const std::string&);

    using HandlerFn = bool (*)(std::stringstream&, const CfgCtx&, const std::string&);

    bool ParseAiChaseRange(std::stringstream& keystream, const CfgCtx& context, const std::string& key)
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value <= 0.0f)
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "AI_ChaseRange <float > 0>");
            return true;
        }

        context.ai->m_chaseRange = value;
        MarkKeyParsed(context, key);

        return true;
    }

    bool ParseAiChaseDeadZone(std::stringstream& keystream, const CfgCtx& context, const std::string& key)
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value < 0.0f)
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "AI_ChaseDeadZone <float >= 0>");
            return true;
        }

        context.ai->m_chaseDeadZone = value;
        MarkKeyParsed(context, key);

        return true;
    }

    bool ParseAiArrivalThreshold(std::stringstream& keystream, const CfgCtx& context, const std::string& key)
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value < 0.0f)
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "AI_ArrivalThreshold <float >= 0>");
            return true;
        }

        context.ai->m_arrivalThreshold = value;
        MarkKeyParsed(context, key);

        return true;
    }

    bool ParseAiIdleInterval(std::stringstream& keystream, const CfgCtx& context, const std::string& key)
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value <= 0.0f)
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "AI_IdleInterval <float > 0>");
            return true;
        }

        context.ai->m_idleInterval = value;
        MarkKeyParsed(context, key);

        return true;
    }

    bool ParseAiPatrolMinDistance(std::stringstream& keystream, const CfgCtx& context, const std::string& key)
    {
        int value = 0;
        if (!TryReadExact(keystream, value) || value <= 0)
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "AI_PatrolMinDistance <int > 0>");
            return true;
        }

        context.ai->m_patrolMinDistance = value;
        MarkKeyParsed(context, key);

        return true;
    }

    bool ParseAiPatrolMaxDistance(std::stringstream& keystream, const CfgCtx& context, const std::string& key)
    {
        int value = 0;
        if (!TryReadExact(keystream, value) || value <= 0)
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "AI_PatrolMaxDistance <int > 0>");
            return true;
        }

        context.ai->m_patrolMaxDistance = value;
        MarkKeyParsed(context, key);

        return true;
    }

    bool ParseAiPatrolDirectionChance(std::stringstream& keystream, const CfgCtx& context, const std::string& key)
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value))
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "AI_PatrolDirectionChance <float 0..1>");
            return true;
        }

        if (value < 0.0f || value > 1.0f)
        {
            EngineLog::Warn(
                "AI_PatrolDirectionChance out of range in '" + context.path + "' at line " +
                std::to_string(context.lineNumber) + ". Clamping to [0,1].");
            if (value < 0.0f) value = 0.0f;
            if (value > 1.0f) value = 1.0f;
        }

        context.ai->m_patrolDirectionChance = value;
        MarkKeyParsed(context, key);

        return true;
    }

    bool ParseAiAttackRange(std::stringstream& keystream, const CfgCtx& context, const std::string& key)
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value <= 0.0f)
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "AI_AttackRange <float > 0>");
            return true;
        }

        context.ai->m_attackRangeX = value;
        context.ai->m_attackRangeY = value;
        MarkKeyParsed(context, key);

        return true;
    }

    bool ParseAiAttackRangeX(std::stringstream& keystream, const CfgCtx& context, const std::string& key)
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value <= 0.0f)
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "AI_AttackRangeX <float > 0>");
            return true;
        }

        context.ai->m_attackRangeX = value;
        MarkKeyParsed(context, key);

        return true;
    }

    bool ParseAiAttackRangeY(std::stringstream& keystream, const CfgCtx& context, const std::string& key)
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value <= 0.0f)
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "AI_AttackRangeY <float > 0>");
            return true;
        }

        context.ai->m_attackRangeY = value;
        MarkKeyParsed(context, key);

        return true;
    }

    // Key -> parser dispatch for AI-related config entries
    const std::unordered_map<std::string, HandlerFn> kAiHandlers = {
        {"AI_ChaseRange", &ParseAiChaseRange},
        {"AI_ChaseDeadZone", &ParseAiChaseDeadZone},
        {"AI_ArrivalThreshold", &ParseAiArrivalThreshold},
        {"AI_IdleInterval", &ParseAiIdleInterval},
        {"AI_PatrolMinDistance", &ParseAiPatrolMinDistance},
        {"AI_PatrolMaxDistance", &ParseAiPatrolMaxDistance},
        {"AI_PatrolDirectionChance", &ParseAiPatrolDirectionChance},
        {"AI_AttackRange", &ParseAiAttackRange},
        {"AI_AttackRangeX", &ParseAiAttackRangeX},
        {"AI_AttackRangeY", &ParseAiAttackRangeY}
    };
}

bool CharacterConfigParser::HandleAiKey(
    const std::string& type,
    std::stringstream& keystream,
    const CfgCtx& context)
{
    const auto it = kAiHandlers.find(type);
    if (it == kAiHandlers.end())
        return false;

    if (!context.ai)
    {
        WarnMissingComponent(context.path, context.lineNumber, type, "CAIPatrol");
        return true;
    }

    return it->second(keystream, context, type);
}