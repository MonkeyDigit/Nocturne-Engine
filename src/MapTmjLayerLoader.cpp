#include <algorithm>
#include <cmath>

#include "MapTmjLoader.h"
#include "EngineLog.h"

namespace
{
    bool LayerHasCollisions(const nlohmann::json& layer)
    {
        bool hasCollisions = true;

        if (!layer.contains("properties") || !layer["properties"].is_array())
            return hasCollisions;

        for (const auto& prop : layer["properties"])
        {
            if (prop.value("name", "") == "HasCollisions")
            {
                hasCollisions = prop.value("value", true);
                break;
            }
        }

        return hasCollisions;
    }

    void AppendTexturedQuad(
        sf::VertexArray& vertexArray,
        const sf::Vector2f& pos,
        const sf::Vector2f& size,
        const sf::Vector2f& texPos)
    {
        vertexArray.append(sf::Vertex(pos, sf::Color::White, texPos));
        vertexArray.append(sf::Vertex({ pos.x + size.x, pos.y }, sf::Color::White, { texPos.x + size.x, texPos.y }));
        vertexArray.append(sf::Vertex({ pos.x, pos.y + size.y }, sf::Color::White, { texPos.x, texPos.y + size.y }));

        vertexArray.append(sf::Vertex({ pos.x, pos.y + size.y }, sf::Color::White, { texPos.x, texPos.y + size.y }));
        vertexArray.append(sf::Vertex({ pos.x + size.x, pos.y }, sf::Color::White, { texPos.x + size.x, texPos.y }));
        vertexArray.append(sf::Vertex({ pos.x + size.x, pos.y + size.y }, sf::Color::White, { texPos.x + size.x, texPos.y + size.y }));
    }

    TileInfo ResolveTileInfo(
        int actualTileId,
        const std::unordered_map<int, TileInfo>& tileTemplates,
        const TileInfo& defaultTileInfo)
    {
        TileInfo result;
        result.id = static_cast<unsigned int>(actualTileId);
        result.friction = defaultTileInfo.friction;
        result.deadly = false;
        result.collision = TileCollision::Solid;

        const auto it = tileTemplates.find(actualTileId);
        if (it != tileTemplates.end())
            result = it->second;

        return result;
    }
}

void MapTmjLoader::ProcessTileLayer(
    Map& map,
    const nlohmann::json& layer,
    const std::unordered_map<int, TileInfo>& tileTemplates,
    unsigned int tilesPerRow,
    const std::string& path)
{
    if (!layer.contains("data") || !layer["data"].is_array()) return;

    const bool layerHasCollisions = LayerHasCollisions(layer);

    // Must stay here: accesses Map private members (friend context).
    auto addCollisionTileIfNeeded =
        [&](const TileInfo& tileInfo,
            const sf::Vector2f& tileSize,
            const sf::Vector2f& texPos,
            unsigned int tileX,
            unsigned int tileY)
        {
            if (!layerHasCollisions || tileInfo.collision == TileCollision::None)
                return;

            auto tile = std::make_unique<Tile>();
            tile->warp = false;
            tile->properties = tileInfo;
            tile->properties.textureRect = sf::IntRect(
                { static_cast<int>(texPos.x), static_cast<int>(texPos.y) },
                { static_cast<int>(tileSize.x), static_cast<int>(tileSize.y) });

            map.m_tileMap.emplace(map.ConvertCoords(tileX, tileY), std::move(tile));
        };

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

        const sf::Vector2f pos(static_cast<float>(x * map.m_tileWidth), static_cast<float>(y * map.m_tileHeight));
        const sf::Vector2f size(static_cast<float>(map.m_tileWidth), static_cast<float>(map.m_tileHeight));
        const sf::Vector2f texPos(
            static_cast<float>((actualID % static_cast<int>(tilesPerRow)) * map.m_tileWidth),
            static_cast<float>((actualID / static_cast<int>(tilesPerRow)) * map.m_tileHeight));

        AppendTexturedQuad(vertexArray, pos, size, texPos);

        const TileInfo currentTileInfo = ResolveTileInfo(actualID, tileTemplates, map.m_defaultTile);
        addCollisionTileIfNeeded(currentTileInfo, size, texPos, x, y);
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
        const std::string name = object.value("name", "Unknown");

        float objX = 0.0f;
        float objY = 0.0f;
        float objW = 0.0f;
        float objH = 0.0f;
        if (!TryReadFiniteObjectRect(object, objX, objY, objW, objH))
        {
            EngineLog::Warn("Skipping object '" + name + "' in '" + path + "' (non-finite coordinates/size).");
            continue;
        }

        const std::string objectType = CanonicalObjectType(ResolveRawObjectType(object));

        if (objectType == "player")
        {
            HandlePlayerObject(
                map, path, name, objX, objY, mapPixelWidth, mapPixelHeight, playerObjectCount);
        }
        else if (objectType == "enemy")
        {
            HandleEnemyObject(map, path, name, objX, objY);
        }
        else if (objectType == "door")
        {
            HandleDoorObject(map, path, objX, objY, objW, objH, doorObjectCount);
        }
        else if (objectType == "trap")
        {
            HandleTrapObject(map, path, objX, objY, objW, objH);
        }
        else
        {
            const std::string objectLabel = name.empty() ? "<unnamed>" : name;

            if (objectType.empty())
            {
                EngineLog::WarnOnce(
                    "map.object.missing_type." + path + "." + objectLabel,
                    "Object '" + objectLabel + "' in map '" + path +
                    "' has no valid type/class/Type property. Object skipped.");
            }
            else
            {
                EngineLog::WarnOnce(
                    "map.object.unknown_type." + path + "." + objectType,
                    "Unknown object type '" + objectType + "' in map '" + path + "'.");
            }
        }
    }
}