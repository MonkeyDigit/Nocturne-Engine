#include <algorithm>
#include <cmath>

#include "MapTmjLoader.h"
#include "EntityManager.h"
#include "SharedContext.h"
#include "EntityBase.h"
#include "EngineLog.h"

void MapTmjLoader::ProcessTileLayer(
    Map& map,
    const nlohmann::json& layer,
    const std::unordered_map<int, TileInfo>& tileTemplates,
    unsigned int tilesPerRow,
    const std::string& path)
{
    if (!layer.contains("data") || !layer["data"].is_array()) return;

    bool layerHasCollisions = true;
    if (layer.contains("properties"))
    {
        for (const auto& prop : layer["properties"])
        {
            if (prop.value("name", "") == "HasCollisions")
                layerHasCollisions = prop.value("value", true);
        }
    }

    sf::VertexArray vertexArray(sf::PrimitiveType::Triangles);
    const auto& data = layer["data"];

    const std::size_t expectedTiles =
        static_cast<std::size_t>(map.m_maxMapSize.x) * static_cast<std::size_t>(map.m_maxMapSize.y);

    if (data.size() != expectedTiles)
    {
        EngineLog::Warn("Tile layer size mismatch: expected " +
            std::to_string(expectedTiles) + ", got " + std::to_string(data.size()) + ".");
    }

    const std::size_t tilesToRead = std::min(expectedTiles, data.size());
    const std::string layerName = layer.value("name", "<unnamed>");
    unsigned int invalidTileTypeCount = 0u;

    for (std::size_t i = 0; i < tilesToRead; ++i)
    {
        const auto& tileValue = data[i];
        if (!tileValue.is_number_integer())
        {
            ++invalidTileTypeCount;
            continue;
        }

        const int tileID = tileValue.get<int>();
        if (tileID <= 0) continue;

        const int actualID = tileID - 1;
        const unsigned int x = static_cast<unsigned int>(i % map.m_maxMapSize.x);
        const unsigned int y = static_cast<unsigned int>(i / map.m_maxMapSize.x);

        sf::Vector2f pos(static_cast<float>(x * map.m_tileWidth), static_cast<float>(y * map.m_tileHeight));
        sf::Vector2f size(static_cast<float>(map.m_tileWidth), static_cast<float>(map.m_tileHeight));
        sf::Vector2f texPos(
            static_cast<float>((actualID % static_cast<int>(tilesPerRow)) * map.m_tileWidth),
            static_cast<float>((actualID / static_cast<int>(tilesPerRow)) * map.m_tileHeight));

        vertexArray.append(sf::Vertex(pos, sf::Color::White, texPos));
        vertexArray.append(sf::Vertex({ pos.x + size.x, pos.y }, sf::Color::White, { texPos.x + size.x, texPos.y }));
        vertexArray.append(sf::Vertex({ pos.x, pos.y + size.y }, sf::Color::White, { texPos.x, texPos.y + size.y }));
        vertexArray.append(sf::Vertex({ pos.x, pos.y + size.y }, sf::Color::White, { texPos.x, texPos.y + size.y }));
        vertexArray.append(sf::Vertex({ pos.x + size.x, pos.y }, sf::Color::White, { texPos.x + size.x, texPos.y }));
        vertexArray.append(sf::Vertex({ pos.x + size.x, pos.y + size.y }, sf::Color::White, { texPos.x + size.x, texPos.y + size.y }));

        TileInfo currentTileInfo;
        currentTileInfo.id = static_cast<unsigned int>(actualID);
        currentTileInfo.friction = map.m_defaultTile.friction;
        currentTileInfo.deadly = false;
        currentTileInfo.collision = TileCollision::Solid;

        auto it = tileTemplates.find(actualID);
        if (it != tileTemplates.end())
            currentTileInfo = it->second;

        if (layerHasCollisions && currentTileInfo.collision != TileCollision::None)
        {
            auto tile = std::make_unique<Tile>();
            tile->warp = false;
            tile->properties = currentTileInfo;
            tile->properties.textureRect = sf::IntRect(
                { static_cast<int>(texPos.x), static_cast<int>(texPos.y) },
                { static_cast<int>(size.x), static_cast<int>(size.y) });

            map.m_tileMap.emplace(map.ConvertCoords(x, y), std::move(tile));
        }
    }

    if (invalidTileTypeCount > 0u)
    {
        EngineLog::Warn(
            "Layer '" + layerName + "' in map '" + path + "' has " +
            std::to_string(invalidTileTypeCount) +
            " non-integer tile entries (ignored).");
    }

    map.m_layerVertices.push_back(vertexArray);
}

void MapTmjLoader::ProcessObjectLayer(
    Map& map,
    const nlohmann::json& layer,
    const std::string& path,
    unsigned int& playerObjectCount,
    unsigned int& doorObjectCount)
{
    if (!layer.contains("objects") || !layer["objects"].is_array()) return;

    const float mapPixelWidth = static_cast<float>(map.m_maxMapSize.x * map.m_tileWidth);
    const float mapPixelHeight = static_cast<float>(map.m_maxMapSize.y * map.m_tileHeight);

    for (const auto& object : layer["objects"])
    {
        std::string typeStr = object.value("class", object.value("type", ""));

        if (typeStr.empty() && object.contains("properties") && object["properties"].is_array())
        {
            for (const auto& prop : object["properties"])
            {
                if (prop.value("name", "") == "Class" || prop.value("name", "") == "Type")
                {
                    typeStr = prop.value("value", "");
                    break;
                }
            }
        }

        const std::string objectType = CanonicalObjectType(typeStr);

        const std::string name = object.value("name", "Unknown");
        const float objX = object.value("x", 0.0f);
        const float objY = object.value("y", 0.0f);
        const float objW = object.value("width", 32.0f);
        const float objH = object.value("height", 32.0f);

        if (!std::isfinite(objX) || !std::isfinite(objY) || !std::isfinite(objW) || !std::isfinite(objH))
        {
            EngineLog::Warn("Skipping object '" + name + "' in '" + path + "' (non-finite coordinates/size).");
            continue;
        }

        if (objectType == "player")
        {
            ++playerObjectCount;
            if (playerObjectCount > 1u)
            {
                EngineLog::WarnOnce(
                    "map.object.player.multiple." + path,
                    "Multiple Player objects found in map '" + path + "'. Using the first one.");
                continue;
            }

            map.m_playerStart = sf::Vector2f(objX, objY);

            if (objX < 0.0f || objY < 0.0f || objX > mapPixelWidth || objY > mapPixelHeight)
            {
                EngineLog::Warn("Player spawn is outside map bounds in '" + path + "'.");
            }

            if (map.m_playerId == -1)
            {
                map.m_playerId = map.m_context.GetEntityManager().Add(EntityType::Player, name);
                if (map.m_playerId < 0)
                {
                    EngineLog::WarnOnce("map.spawn.player.failed", "Failed to spawn player from map object");
                    continue;
                }
            }

            EntityBase* player = map.m_context.GetEntityManager().Find(static_cast<unsigned int>(map.m_playerId));
            if (player)
                player->SetPosition(map.m_playerStart);
        }
        else if (objectType == "enemy")
        {
            if (name.empty() || name == "Unknown")
            {
                EngineLog::Warn("Enemy object missing type id in name field in '" + path + "'. Object skipped.");
                continue;
            }

            int enemyId = map.m_context.GetEntityManager().Add(EntityType::Enemy, name);
            if (enemyId < 0)
            {
                EngineLog::WarnOnce("map.spawn.enemy.failed", "Failed to spawn enemy from map object");
                continue;
            }

            EntityBase* enemy = map.m_context.GetEntityManager().Find(static_cast<unsigned int>(enemyId));
            if (enemy)
                enemy->SetPosition(sf::Vector2f(objX, objY));
        }
        else if (objectType == "door")
        {
            if (objW <= 0.0f || objH <= 0.0f)
            {
                EngineLog::Warn("Door object with non-positive size in '" + path + "'. Object skipped.");
                continue;
            }

            ++doorObjectCount;
            if (doorObjectCount > 1u)
            {
                EngineLog::WarnOnce(
                    "map.object.door.multiple." + path,
                    "Multiple Door objects found in map '" + path + "'. Using the first one.");
                continue;
            }

            map.m_doorRect = { {objX, objY}, {objW, objH} };
        }
        else if (objectType == "trap")
        {
            if (objW <= 0.0f || objH <= 0.0f)
            {
                EngineLog::Warn("Trap object with non-positive size in '" + path + "'. Object skipped.");
                continue;
            }

            map.m_traps.push_back({ {objX, objY}, {objW, objH} });
        }
        else if (!objectType.empty())
        {
            EngineLog::WarnOnce(
                "map.object.unknown_type." + path + "." + objectType,
                "Unknown object type '" + objectType + "' in map '" + path + "'.");
        }
    }
}