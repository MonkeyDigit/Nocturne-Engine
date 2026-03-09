#include "CharacterConfigParser.h"

#include "CState.h"
#include "ConfigParseUtils.h"

namespace
{
    using ParseUtils::TryReadExact;
}

bool CharacterConfigParser::HandleStateCombatKey(
    const std::string& type,
    std::stringstream& keystream,
    const ParseContext& context)
{
    const bool isStateCombatKey =
        (type == "AttackDamage") ||
        (type == "AttackKnockback") ||
        (type == "TouchDamage") ||
        (type == "InvulnerabilityTime");

    if (isStateCombatKey && !context.state)
    {
        WarnMissingComponent(context.path, context.lineNumber, type, "CState");
        return true;
    }

    if (type == "AttackDamage")
    {
        int damage = 0;
        if (!TryReadExact(keystream, damage) || damage <= 0)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "AttackDamage <int > 0>");
            return true;
        }

        context.state->SetAttackDamage(damage);
        return true;
    }

    if (type == "AttackKnockback")
    {
        float x = 0.0f, y = 0.0f;
        if (!TryReadExact(keystream, x, y) || x < 0.0f)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "AttackKnockback <x >= 0> <y>");
            return true;
        }

        context.state->SetAttackKnockback(x, y);
        return true;
    }

    if (type == "TouchDamage")
    {
        int damage = 0;
        if (!TryReadExact(keystream, damage) || damage < 0)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "TouchDamage <int >= 0>");
            return true;
        }

        context.state->SetTouchDamage(damage);
        return true;
    }

    if (type == "InvulnerabilityTime")
    {
        float seconds = 0.0f;
        if (!TryReadExact(keystream, seconds) || seconds < 0.0f)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "InvulnerabilityTime <float >= 0>");
            return true;
        }

        context.state->SetInvulnerabilityTime(seconds);
        return true;
    }

    return false;
}