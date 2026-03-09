#include "CharacterConfigParser.h"

#include "CSprite.h"
#include "CState.h"
#include "CTransform.h"
#include "CBoxCollider.h"
#include "EngineLog.h"
#include "ConfigParseUtils.h"

namespace
{
    using ParseUtils::TryReadExact;
}

void CharacterConfigParser::WarnInvalidValue(
    const std::string& path,
    unsigned int lineNumber,
    const std::string& key,
    const std::string& expected)
{
    EngineLog::Warn(
        "Invalid '" + key + "' in '" + path + "' at line " +
        std::to_string(lineNumber) + " (expected: " + expected + ")");
}

void CharacterConfigParser::WarnMissingComponent(
    const std::string& path,
    unsigned int lineNumber,
    const std::string& key,
    const std::string& componentName)
{
    EngineLog::WarnOnce(
        "char.missing_component." + path + "." + key + "." + componentName,
        "Key '" + key + "' in character file '" + path + "' at line " +
        std::to_string(lineNumber) + " requires component '" + componentName +
        "', but the entity does not have it.");
}

bool CharacterConfigParser::HandleCoreKey(
    const std::string& type,
    std::stringstream& keystream,
    const ParseContext& context)
{
    if (type == "Name")
    {
        std::string name;
        if (!TryReadExact(keystream, name))
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "Name <string>");
            return true;
        }

        context.entityName = name;
        return true;
    }

    if (type == "Spritesheet")
    {
        std::string spritePath;
        if (!TryReadExact(keystream, spritePath))
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "Spritesheet <path>");
            return true;
        }

        if (!context.sprite)
        {
            WarnMissingComponent(context.path, context.lineNumber, type, "CSprite");
            return true;
        }

        context.sprite->Load(spritePath);
        return true;
    }

    if (type == "Hitpoints")
    {
        int maxHitPoints = 0;
        if (!TryReadExact(keystream, maxHitPoints) || maxHitPoints <= 0)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "Hitpoints <int > 0>");
            return true;
        }

        if (!context.state)
        {
            WarnMissingComponent(context.path, context.lineNumber, type, "CState");
            return true;
        }

        context.state->SetHitPoints(maxHitPoints);
        return true;
    }

    if (type == "BoundingBox")
    {
        float x = 0.0f, y = 0.0f;
        if (!TryReadExact(keystream, x, y) || x <= 0.0f || y <= 0.0f)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "BoundingBox <width > 0> <height > 0>");
            return true;
        }

        if (!context.transform)
            WarnMissingComponent(context.path, context.lineNumber, type, "CTransform");
        else
            context.transform->SetSize(x, y);

        if (!context.collider)
            WarnMissingComponent(context.path, context.lineNumber, type, "CBoxCollider");
        else
            context.collider->SetAABB(sf::FloatRect({ 0.0f, 0.0f }, { x, y }));

        return true;
    }

    if (type == "DamageBox")
    {
        float offsetX = 0.0f, offsetY = 0.0f, width = 0.0f, height = 0.0f;
        if (!TryReadExact(keystream, offsetX, offsetY, width, height) ||
            width <= 0.0f || height <= 0.0f)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "DamageBox <offsetX> <offsetY> <width > 0> <height > 0>");
            return true;
        }

        if (!context.collider)
        {
            WarnMissingComponent(context.path, context.lineNumber, type, "CBoxCollider");
            return true;
        }

        context.collider->SetAttackAABB(sf::FloatRect({ 0.0f, 0.0f }, { width, height }));
        context.collider->SetAttackAABBOffset(sf::Vector2f(offsetX, offsetY));
        return true;
    }

    if (type == "Speed")
    {
        float x = 0.0f, y = 0.0f;
        if (!TryReadExact(keystream, x, y) || x <= 0.0f || y <= 0.0f)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "Speed <x > 0> <y > 0>");
            return true;
        }

        if (!context.transform)
        {
            WarnMissingComponent(context.path, context.lineNumber, type, "CTransform");
            return true;
        }

        context.transform->SetSpeed(x, y);
        return true;
    }

    if (type == "MaxVelocity")
    {
        float x = 0.0f, y = 0.0f;
        if (!TryReadExact(keystream, x, y) || x <= 0.0f || y <= 0.0f)
        {
            WarnInvalidValue(context.path, context.lineNumber, type, "MaxVelocity <x > 0> <y > 0>");
            return true;
        }

        if (!context.transform)
        {
            WarnMissingComponent(context.path, context.lineNumber, type, "CTransform");
            return true;
        }

        context.transform->SetMaxVelocity(x, y);
        return true;
    }

    return false;
}