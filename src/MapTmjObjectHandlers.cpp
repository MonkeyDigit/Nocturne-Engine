#include <cmath>
#include <string>

#include "MapTmjLoader.h"
#include "EntityManager.h"
#include "SharedContext.h"
#include "EntityBase.h"
#include "EngineLog.h"

std::string MapTmjLoader::ResolveRawObjectType(const nlohmann::json& object)
{
    std::string typeStr = object.value("class", object.value("type", ""));
    if (!typeStr.empty())
        return typeStr;

    if (object.contains("properties") && object["properties"].is_array())
    {
        for (const auto& prop : object["properties"])
        {
            const std::string propName = prop.value("name", "");
            if (propName == "Class" || propName == "Type")
                return prop.value("value", "");
        }
    }

    // Compatibility fallback: accept well-known object names as semantic type
    const std::string nameFallback = object.value("name", "");
    const std::string canonicalName = CanonicalObjectType(nameFallback);

    if (canonicalName == "player" ||
        canonicalName == "enemy" ||
        canonicalName == "door" ||
        canonicalName == "trap")
    {
        return nameFallback;
    }

    return "";
}

bool MapTmjLoader::TryReadFiniteObjectRect(
    const nlohmann::json& object,
    float& outX,
    float& outY,
    float& outW,
    float& outH)
{
    outX = object.value("x", 0.0f);
    outY = object.value("y", 0.0f);
    outW = object.value("width", 32.0f);
    outH = object.value("height", 32.0f);

    return std::isfinite(outX) &&
        std::isfinite(outY) &&
        std::isfinite(outW) &&
        std::isfinite(outH);
}

void MapTmjLoader::HandlePlayerObject(
    Map& map,
    const std::string& path,
    const std::string& name,
    float objX,
    float objY,
    float mapPixelWidth,
    float mapPixelHeight,
    unsigned int& playerObjectCount)
{
    ++playerObjectCount;
    if (playerObjectCount > 1u)
    {
        EngineLog::WarnOnce(
            "map.object.player.multiple." + path,
            "Multiple Player objects found in map '" + path + "'. Using the first one.");
        return;
    }

    map.m_playerStart = sf::Vector2f(objX, objY);

    if (objX < 0.0f || objY < 0.0f || objX > mapPixelWidth || objY > mapPixelHeight)
        EngineLog::Warn("Player spawn is outside map bounds in '" + path + "'.");

    if (map.m_playerId == -1)
    {
        map.m_playerId = map.m_context.GetEntityManager().Add(EntityType::Player, name);
        if (map.m_playerId < 0)
        {
            EngineLog::WarnOnce("map.spawn.player.failed", "Failed to spawn player from map object");
            return;
        }
    }

    EntityBase* player = map.m_context.GetEntityManager().Find(static_cast<unsigned int>(map.m_playerId));
    if (player)
        player->SetPosition(map.m_playerStart);
}

void MapTmjLoader::HandleEnemyObject(
    Map& map,
    const std::string& path,
    const std::string& name,
    float objX,
    float objY)
{
    if (name.empty() || name == "Unknown")
    {
        EngineLog::Warn("Enemy object missing type id in name field in '" + path + "'. Object skipped.");
        return;
    }

    const int enemyId = map.m_context.GetEntityManager().Add(EntityType::Enemy, name);
    if (enemyId < 0)
    {
        EngineLog::WarnOnce("map.spawn.enemy.failed", "Failed to spawn enemy from map object");
        return;
    }

    EntityBase* enemy = map.m_context.GetEntityManager().Find(static_cast<unsigned int>(enemyId));
    if (enemy)
        enemy->SetPosition(sf::Vector2f(objX, objY));
}

void MapTmjLoader::HandleDoorObject(
    Map& map,
    const std::string& path,
    float objX,
    float objY,
    float objW,
    float objH,
    unsigned int& doorObjectCount)
{
    if (objW <= 0.0f || objH <= 0.0f)
    {
        EngineLog::Warn("Door object with non-positive size in '" + path + "'. Object skipped.");
        return;
    }

    ++doorObjectCount;
    if (doorObjectCount > 1u)
    {
        EngineLog::WarnOnce(
            "map.object.door.multiple." + path,
            "Multiple Door objects found in map '" + path + "'. Using the first one.");
        return;
    }

    map.m_doorRect = { {objX, objY}, {objW, objH} };
}

void MapTmjLoader::HandleTrapObject(
    Map& map,
    const std::string& path,
    float objX,
    float objY,
    float objW,
    float objH)
{
    if (objW <= 0.0f || objH <= 0.0f)
    {
        EngineLog::Warn("Trap object with non-positive size in '" + path + "'. Object skipped.");
        return;
    }

    map.m_traps.push_back({ {objX, objY}, {objW, objH} });
}