#include <unordered_map>

#include "CharacterConfigParser.h"
#include "CController.h"
#include "ConfigParseUtils.h"

namespace
{
    using ParseContext = CharacterConfigParser::ParseContext;
    using ParseUtils::TryReadExact;
    using HandlerFn = bool (*)(std::stringstream&, const ParseContext&, const std::string&);

    bool ParseJumpVelocity(std::stringstream& keystream, const ParseContext& context, const std::string& key)
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value <= 0.0f)
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "JumpVelocity <float > 0>");
            return true;
        }

        context.controller->m_jumpVelocity = value;
        return true;
    }

    bool ParseRangedCooldown(std::stringstream& keystream, const ParseContext& context, const std::string& key)
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value < 0.0f)
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "RangedCooldown <float >= 0>");
            return true;
        }

        context.controller->m_rangedCooldown = value;
        return true;
    }

    bool ParseRangedSpeed(std::stringstream& keystream, const ParseContext& context, const std::string& key)
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value <= 0.0f)
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "RangedSpeed <float > 0>");
            return true;
        }

        context.controller->m_rangedSpeed = value;
        return true;
    }

    bool ParseRangedLifetime(std::stringstream& keystream, const ParseContext& context, const std::string& key)
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value <= 0.0f)
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "RangedLifetime <float > 0>");
            return true;
        }

        context.controller->m_rangedLifetime = value;
        return true;
    }

    bool ParseRangedDamage(std::stringstream& keystream, const ParseContext& context, const std::string& key)
    {
        int value = 0;
        if (!TryReadExact(keystream, value) || value <= 0)
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "RangedDamage <int > 0>");
            return true;
        }

        context.controller->m_rangedDamage = value;
        return true;
    }

    bool ParseRangedSpawnOffset(std::stringstream& keystream, const ParseContext& context, const std::string& key)
    {
        float offsetX = 0.0f;
        float offsetY = 0.0f;
        if (!TryReadExact(keystream, offsetX, offsetY) || offsetX < 0.0f)
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "RangedSpawnOffset <x >= 0> <y>");
            return true;
        }

        context.controller->m_rangedSpawnOffsetX = offsetX;
        context.controller->m_rangedSpawnOffsetY = offsetY;
        return true;
    }

    bool ParseRangedSize(std::stringstream& keystream, const ParseContext& context, const std::string& key)
    {
        float sizeX = 0.0f;
        float sizeY = 0.0f;
        if (!TryReadExact(keystream, sizeX, sizeY) || sizeX <= 0.0f || sizeY <= 0.0f)
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "RangedSize <x > 0> <y > 0>");
            return true;
        }

        context.controller->m_rangedSizeX = sizeX;
        context.controller->m_rangedSizeY = sizeY;
        return true;
    }

    bool ParseRangedSheet(std::stringstream& keystream, const ParseContext& context, const std::string& key)
    {
        std::string sheetPath;
        if (!TryReadExact(keystream, sheetPath) || sheetPath.empty())
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "RangedSheet <path>");
            return true;
        }

        context.controller->m_rangedSheetPath = sheetPath;
        return true;
    }

    bool ParseRangedAnimation(std::stringstream& keystream, const ParseContext& context, const std::string& key)
    {
        std::string animationName;
        if (!TryReadExact(keystream, animationName) || animationName.empty())
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "RangedAnimation <name>");
            return true;
        }

        context.controller->m_rangedAnimation = animationName;
        return true;
    }

    bool ParseRangedEnabled(std::stringstream& keystream, const ParseContext& context, const std::string& key)
    {
        int value = 0;
        if (!TryReadExact(keystream, value) || (value != 0 && value != 1))
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "RangedEnabled <0|1>");
            return true;
        }

        context.controller->m_rangedEnabled = (value == 1);
        return true;
    }

    bool ParseCoyoteTime(std::stringstream& keystream, const ParseContext& context, const std::string& key)
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value < 0.0f)
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "CoyoteTime <float >= 0>");
            return true;
        }

        context.controller->m_coyoteTimeWindow = value;
        return true;
    }

    bool ParseJumpBufferTime(std::stringstream& keystream, const ParseContext& context, const std::string& key)
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value < 0.0f)
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "JumpBufferTime <float >= 0>");
            return true;
        }

        context.controller->m_jumpBufferWindow = value;
        return true;
    }

    bool ParseAttackCooldown(std::stringstream& keystream, const ParseContext& context, const std::string& key)
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value < 0.0f)
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "AttackCooldown <float >= 0>");
            return true;
        }

        context.controller->m_attackCooldown = value;
        return true;
    }

    bool ParseJumpCancelMultiplier(std::stringstream& keystream, const ParseContext& context, const std::string& key)
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value <= 0.0f || value > 1.0f)
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "JumpCancelMultiplier <0 < value <= 1>");
            return true;
        }

        context.controller->m_jumpCancelMultiplier = value;
        return true;
    }

    bool ParseVerticalAirThreshold(std::stringstream& keystream, const ParseContext& context, const std::string& key)
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value < 0.0f)
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "VerticalAirThreshold <float >= 0>");
            return true;
        }

        context.controller->m_verticalAirThreshold = value;
        return true;
    }

    bool ParseHorizontalWalkThreshold(std::stringstream& keystream, const ParseContext& context, const std::string& key)
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value < 0.0f)
        {
            CharacterConfigParser::WarnInvalidValue(context.path, context.lineNumber, key, "HorizontalWalkThreshold <float >= 0>");
            return true;
        }

        context.controller->m_horizontalWalkThreshold = value;
        return true;
    }

    // Key -> parser dispatch for controller-related config entries
    const std::unordered_map<std::string, HandlerFn> kControllerHandlers = {
        {"JumpVelocity", &ParseJumpVelocity},
        {"RangedCooldown", &ParseRangedCooldown},
        {"RangedSpeed", &ParseRangedSpeed},
        {"RangedLifetime", &ParseRangedLifetime},
        {"RangedDamage", &ParseRangedDamage},
        {"RangedSpawnOffset", &ParseRangedSpawnOffset},
        {"RangedSize", &ParseRangedSize},
        {"RangedSheet", &ParseRangedSheet},
        {"RangedAnimation", &ParseRangedAnimation},
        {"RangedEnabled", &ParseRangedEnabled},
        {"CoyoteTime", &ParseCoyoteTime},
        {"JumpBufferTime", &ParseJumpBufferTime},
        {"AttackCooldown", &ParseAttackCooldown},
        {"JumpCancelMultiplier", &ParseJumpCancelMultiplier},
        {"VerticalAirThreshold", &ParseVerticalAirThreshold},
        {"HorizontalWalkThreshold", &ParseHorizontalWalkThreshold}
    };
}

bool CharacterConfigParser::HandleControllerKey(
    const std::string& type,
    std::stringstream& keystream,
    const ParseContext& context)
{
    const auto it = kControllerHandlers.find(type);
    if (it == kControllerHandlers.end())
        return false;

    if (!context.controller)
    {
        WarnMissingComponent(context.path, context.lineNumber, type, "CController");
        return true;
    }

    return it->second(keystream, context, type);
}