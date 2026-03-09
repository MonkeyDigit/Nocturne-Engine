#include "CharacterConfigParser.h"

#include "CController.h"
#include "ConfigParseUtils.h"

namespace
{
    using ParseUtils::TryReadExact;
}

bool CharacterConfigParser::HandleControllerKey(
    const std::string& type,
    std::stringstream& keystream,
    const ParseContext& context)
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

    if (isControllerKey && !context.controller)
    {
        WarnMissingComponent(context.path, context.lineNumber, type, "CController");
        return true;
    }

    if (type == "JumpVelocity")
    {
        float jv = 0.0f;
        if (!TryReadExact(keystream, jv) || jv <= 0.0f)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "JumpVelocity <float > 0>");
            return true;
        }

        context.controller->m_jumpVelocity = jv;
        return true;
    }

    if (type == "RangedCooldown")
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value < 0.0f)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "RangedCooldown <float >= 0>");
            return true;
        }

        context.controller->m_rangedCooldown = value;
        return true;
    }

    if (type == "RangedSpeed")
    {
        float speed = 0.0f;
        if (!TryReadExact(keystream, speed) || speed <= 0.0f)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "RangedSpeed <float > 0>");
            return true;
        }

        context.controller->m_rangedSpeed = speed;
        return true;
    }

    if (type == "RangedLifetime")
    {
        float lifetime = 0.0f;
        if (!TryReadExact(keystream, lifetime) || lifetime <= 0.0f)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "RangedLifetime <float > 0>");
            return true;
        }

        context.controller->m_rangedLifetime = lifetime;
        return true;
    }

    if (type == "RangedDamage")
    {
        int damage = 0;
        if (!TryReadExact(keystream, damage) || damage <= 0)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "RangedDamage <int > 0>");
            return true;
        }

        context.controller->m_rangedDamage = damage;
        return true;
    }

    if (type == "RangedSpawnOffset")
    {
        float offsetX = 0.0f, offsetY = 0.0f;
        if (!TryReadExact(keystream, offsetX, offsetY) || offsetX < 0.0f)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "RangedSpawnOffset <x >= 0> <y>");
            return true;
        }

        context.controller->m_rangedSpawnOffsetX = offsetX;
        context.controller->m_rangedSpawnOffsetY = offsetY;
        return true;
    }

    if (type == "RangedSize")
    {
        float sx = 0.0f, sy = 0.0f;
        if (!TryReadExact(keystream, sx, sy) || sx <= 0.0f || sy <= 0.0f)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "RangedSize <x > 0> <y > 0>");
            return true;
        }

        context.controller->m_rangedSizeX = sx;
        context.controller->m_rangedSizeY = sy;
        return true;
    }

    if (type == "RangedSheet")
    {
        std::string sheetPath;
        if (!TryReadExact(keystream, sheetPath) || sheetPath.empty())
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "RangedSheet <path>");
            return true;
        }

        context.controller->m_rangedSheetPath = sheetPath;
        return true;
    }

    if (type == "RangedAnimation")
    {
        std::string animName;
        if (!TryReadExact(keystream, animName) || animName.empty())
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "RangedAnimation <name>");
            return true;
        }

        context.controller->m_rangedAnimation = animName;
        return true;
    }

    if (type == "RangedEnabled")
    {
        int value = 0;
        if (!TryReadExact(keystream, value) || (value != 0 && value != 1))
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "RangedEnabled <0|1>");
            return true;
        }

        context.controller->m_rangedEnabled = (value == 1);
        return true;
    }

    if (type == "CoyoteTime")
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value < 0.0f)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "CoyoteTime <float >= 0>");
            return true;
        }

        context.controller->m_coyoteTimeWindow = value;
        return true;
    }

    if (type == "JumpBufferTime")
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value < 0.0f)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "JumpBufferTime <float >= 0>");
            return true;
        }

        context.controller->m_jumpBufferWindow = value;
        return true;
    }

    if (type == "AttackCooldown")
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value < 0.0f)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "AttackCooldown <float >= 0>");
            return true;
        }

        context.controller->m_attackCooldown = value;
        return true;
    }

    if (type == "JumpCancelMultiplier")
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value <= 0.0f || value > 1.0f)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "JumpCancelMultiplier <0 < value <= 1>");
            return true;
        }

        context.controller->m_jumpCancelMultiplier = value;
        return true;
    }

    if (type == "VerticalAirThreshold")
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value < 0.0f)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "VerticalAirThreshold <float >= 0>");
            return true;
        }

        context.controller->m_verticalAirThreshold = value;
        return true;
    }

    if (type == "HorizontalWalkThreshold")
    {
        float value = 0.0f;
        if (!TryReadExact(keystream, value) || value < 0.0f)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "HorizontalWalkThreshold <float >= 0>");
            return true;
        }

        context.controller->m_horizontalWalkThreshold = value;
        return true;
    }

    return false;
}