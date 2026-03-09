#include "CharacterConfigParser.h"

#include "CAIPatrol.h"
#include "EngineLog.h"
#include "ConfigParseUtils.h"

namespace
{
    using ParseUtils::TryReadExact;
}

bool CharacterConfigParser::HandleAiKey(
    const std::string& type,
    std::stringstream& keystream,
    const ParseContext& context)
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

    if (isAiKey && !context.ai)
    {
        WarnMissingComponent(context.path, context.lineNumber, type, "CAIPatrol");
        return true;
    }

    if (type == "AI_ChaseRange")
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value <= 0.0f)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "AI_ChaseRange <float > 0>");
            return true;
        }

        context.ai->m_chaseRange = value;
        return true;
    }

    if (type == "AI_ChaseDeadZone")
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value < 0.0f)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "AI_ChaseDeadZone <float >= 0>");
            return true;
        }

        context.ai->m_chaseDeadZone = value;
        return true;
    }

    if (type == "AI_ArrivalThreshold")
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value < 0.0f)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "AI_ArrivalThreshold <float >= 0>");
            return true;
        }

        context.ai->m_arrivalThreshold = value;
        return true;
    }

    if (type == "AI_IdleInterval")
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value <= 0.0f)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "AI_IdleInterval <float > 0>");
            return true;
        }

        context.ai->m_idleInterval = value;
        return true;
    }

    if (type == "AI_PatrolMinDistance")
    {
        int value = 0;
        if (!TryReadExact(keystream, value) || value <= 0)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "AI_PatrolMinDistance <int > 0>");
            return true;
        }

        context.ai->m_patrolMinDistance = value;
        return true;
    }

    if (type == "AI_PatrolMaxDistance")
    {
        int value = 0;
        if (!TryReadExact(keystream, value) || value <= 0)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "AI_PatrolMaxDistance <int > 0>");
            return true;
        }

        context.ai->m_patrolMaxDistance = value;
        return true;
    }

    if (type == "AI_PatrolDirectionChance")
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value))
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "AI_PatrolDirectionChance <float 0..1>");
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
        return true;
    }

    if (type == "AI_AttackRange")
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value <= 0.0f)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "AI_AttackRange <float > 0>");
            return true;
        }

        context.ai->m_attackRangeX = value;
        context.ai->m_attackRangeY = value;
        return true;
    }

    if (type == "AI_AttackRangeX")
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value <= 0.0f)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "AI_AttackRangeX <float > 0>");
            return true;
        }

        context.ai->m_attackRangeX = value;
        return true;
    }

    if (type == "AI_AttackRangeY")
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value <= 0.0f)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "AI_AttackRangeY <float > 0>");
            return true;
        }

        context.ai->m_attackRangeY = value;
        return true;
    }

    return false;
}