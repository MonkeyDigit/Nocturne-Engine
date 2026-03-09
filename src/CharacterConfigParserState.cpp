#include <unordered_map>

#include "CharacterConfigParser.h"
#include "CState.h"
#include "ConfigParseUtils.h"

namespace
{
    using ParseUtils::TryReadExact;
    using CfgCtx = CharacterConfigParser::ParseContext;
    using HandlerFn = bool (*)(std::stringstream&, const CfgCtx&, const std::string&);

    bool ParseAttackDamage(std::stringstream& keystream, const CfgCtx& context, const std::string& key)
    {
        int damage = 0;
        if (!TryReadExact(keystream, damage) || damage <= 0)
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "AttackDamage <int > 0>");
            return true;
        }

        context.state->SetAttackDamage(damage);
        MarkKeyParsed(context, key);

        return true;
    }

    bool ParseAttackKnockback(std::stringstream& keystream, const CfgCtx& context, const std::string& key)
    {
        float x = 0.0f;
        float y = 0.0f;
        if (!TryReadExact(keystream, x, y) || x < 0.0f)
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "AttackKnockback <x >= 0> <y>");
            return true;
        }

        context.state->SetAttackKnockback(x, y);
        MarkKeyParsed(context, key);

        return true;
    }

    bool ParseTouchDamage(std::stringstream& keystream, const CfgCtx& context, const std::string& key)
    {
        int damage = 0;
        if (!TryReadExact(keystream, damage) || damage < 0)
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "TouchDamage <int >= 0>");
            return true;
        }

        context.state->SetTouchDamage(damage);
        MarkKeyParsed(context, key);

        return true;
    }

    bool ParseInvulnerabilityTime(std::stringstream& keystream, const CfgCtx& context, const std::string& key)
    {
        float seconds = 0.0f;
        if (!TryReadExact(keystream, seconds) || seconds < 0.0f)
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "InvulnerabilityTime <float >= 0>");
            return true;
        }

        context.state->SetInvulnerabilityTime(seconds);
        MarkKeyParsed(context, key);

        return true;
    }

    // Key -> parser dispatch for state/combat-related config entries
    const std::unordered_map<std::string, HandlerFn> kStateCombatHandlers = {
        {"AttackDamage", &ParseAttackDamage},
        {"AttackKnockback", &ParseAttackKnockback},
        {"TouchDamage", &ParseTouchDamage},
        {"InvulnerabilityTime", &ParseInvulnerabilityTime}
    };
}

bool CharacterConfigParser::HandleStateCombatKey(
    const std::string& type,
    std::stringstream& keystream,
    const CfgCtx& context)
{
    const auto it = kStateCombatHandlers.find(type);
    if (it == kStateCombatHandlers.end())
        return false;

    if (!context.state)
    {
        WarnMissingComponent(context.path, context.lineNumber, type, "CState");
        return true;
    }

    return it->second(keystream, context, type);
}