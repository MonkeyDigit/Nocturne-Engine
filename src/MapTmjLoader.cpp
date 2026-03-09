#include <algorithm>
#include <cctype>
#include <fstream>

#include "MapTmjLoader.h"
#include "SharedContext.h"
#include "TextureManager.h"
#include "Utilities.h"
#include "EngineLog.h"

std::string MapTmjLoader::ToLowerCopy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string MapTmjLoader::CanonicalObjectType(std::string rawType)
{
    rawType = ToLowerCopy(std::move(rawType));

    if (rawType == "player" || rawType == "spawn_player") return "player";
    if (rawType == "enemy") return "enemy";
    if (rawType == "door" || rawType == "exit") return "door";
    if (rawType == "trap" || rawType == "hazard") return "trap";

    return rawType;
}

bool MapTmjLoader::LoadJson(const std::string& path, nlohmann::json& outData)
{
    std::ifstream file(Utils::GetWorkingDirectory() + path);
    if (!file.is_open())
    {
        EngineLog::Error("Failed loading map file: " + path);
        return false;
    }

    try
    {
        file >> outData;
        return true;
    }
    catch (const nlohmann::json::parse_error& e)
    {
        EngineLog::Error(std::string("JSON Parse Error: ") + e.what());
        return false;
    }
}

void MapTmjLoader::ResetPerMapDefaults(Map& map)
{
    // Reset per-map defaults to avoid carrying values from previous maps.
    map.m_mapGravity = 512.0f;
    map.m_defaultTile.friction = { 0.9f, 0.9f };
    map.m_nextMap.clear();
    map.m_playerStart = { 0.0f, 0.0f };
    map.m_doorRect = sf::FloatRect();
    map.m_traps.clear();
}

bool MapTmjLoader::ReadMapGeometry(Map& map, const nlohmann::json& mapData, const std::string& path)
{
    map.m_maxMapSize.x = mapData.value("width", 32u);
    map.m_maxMapSize.y = mapData.value("height", 32u);

    map.m_tileWidth = mapData.value("tilewidth", 16u);
    map.m_tileHeight = mapData.value("tileheight", 16u);

    if (map.m_tileWidth == 0u || map.m_tileHeight == 0u)
    {
        EngineLog::Error("Invalid map tile size in '" + path + "' (tilewidth/tileheight must be > 0).");
        return false;
    }

    return true;
}

unsigned int MapTmjLoader::PrepareTilesetTexture(Map& map)
{
    // Release any past map tileset texture.
    if (map.m_tileTexture)
    {
        map.m_context.m_textureManager.ReleaseResource("MapTileSet");
        map.m_tileTexture = nullptr;
    }

    if (map.m_context.m_textureManager.RequireResource("MapTileSet"))
        map.m_tileTexture = map.m_context.m_textureManager.GetResource("MapTileSet");

    unsigned int tilesPerRow = 1u;

    if (!map.m_tileTexture)
    {
        EngineLog::WarnOnce("map.tileset.missing", "Map tileset texture 'MapTileSet' is missing or failed to load.");
    }
    else
    {
        const unsigned int textureWidth = map.m_tileTexture->getSize().x;
        if (textureWidth < map.m_tileWidth)
        {
            EngineLog::WarnOnce(
                "map.tileset.too_narrow",
                "Tileset texture width is smaller than tile width. Falling back to one column.");
        }
        else
        {
            tilesPerRow = textureWidth / map.m_tileWidth;
            if (tilesPerRow == 0u) tilesPerRow = 1u;
        }
    }

    return tilesPerRow;
}

void MapTmjLoader::LoadMapPropertiesAndBackgrounds(Map& map, const nlohmann::json& mapData)
{
    bool hasMapFrictionX = false;
    bool hasMapFrictionY = false;

    if (!mapData.contains("properties") || !mapData["properties"].is_array())
        return;

    for (const auto& prop : mapData["properties"])
    {
        const std::string propName = prop.value("name", "");

        if (propName == "Gravity")
        {
            map.m_mapGravity = prop.value("value", 512.0f);
        }
        else if (propName == "NextMap")
        {
            map.m_nextMap = prop.value("value", "");
        }
        else if (propName == "Friction_X")
        {
            map.m_defaultTile.friction.x = prop.value("value", 0.9f);
            hasMapFrictionX = true;
        }
        else if (propName == "Friction_Y")
        {
            map.m_defaultTile.friction.y = prop.value("value", 0.9f);
            hasMapFrictionY = true;
        }
        else if (propName.find("Bg_") == 0)
        {
            const std::string texName = prop.value("value", "");
            if (texName.empty())
            {
                EngineLog::Warn("Map background property has empty texture id.");
                continue;
            }

            if (!map.m_context.m_textureManager.RequireResource(texName))
            {
                EngineLog::WarnOnce(
                    "map.bg.require_failed." + texName,
                    "Failed to require background texture '" + texName + "'");
                continue;
            }

            sf::Texture* tex = map.m_context.m_textureManager.GetResource(texName);
            if (!tex)
            {
                EngineLog::WarnOnce(
                    "map.bg.null_after_require." + texName,
                    "Background texture '" + texName + "' is null after successful require.");
                continue;
            }

            map.m_backgrounds.emplace_back(*tex, texName);
        }
    }

    // Backward compatibility: if only one axis exists, mirror it.
    if (hasMapFrictionX && !hasMapFrictionY) map.m_defaultTile.friction.y = map.m_defaultTile.friction.x;
    if (!hasMapFrictionX && hasMapFrictionY) map.m_defaultTile.friction.x = map.m_defaultTile.friction.y;
}

void MapTmjLoader::BuildTileTemplates(
    Map& map,
    const nlohmann::json& mapData,
    std::unordered_map<int, TileInfo>& tileTemplates)
{
    if (!mapData.contains("tilesets") || !mapData["tilesets"].is_array())
        return;

    for (const auto& tileset : mapData["tilesets"])
    {
        if (tileset.contains("source") &&
            tileset["source"].get<std::string>().find(".tsx") != std::string::npos)
        {
            EngineLog::WarnOnce(
                "map.tileset.external_tsx",
                "Tileset is external (.tsx). Please EMBED it in Tiled to read tile properties.");
        }

        if (!tileset.contains("tiles") || !tileset["tiles"].is_array())
            continue;

        for (const auto& tile : tileset["tiles"])
        {
            const int id = tile.value("id", -1);
            if (id < 0 || !tile.contains("properties") || !tile["properties"].is_array())
                continue;

            TileInfo infoTemplate;
            infoTemplate.id = static_cast<unsigned int>(id);
            infoTemplate.friction = map.m_defaultTile.friction;
            infoTemplate.deadly = false;
            infoTemplate.collision = TileCollision::Solid;

            bool hasTileFrictionX = false;
            bool hasTileFrictionY = false;

            for (const auto& prop : tile["properties"])
            {
                const std::string pName = prop.value("name", "");

                if (pName == "Deadly")
                {
                    infoTemplate.deadly = prop.value("value", false);
                }
                else if (pName == "Friction_X")
                {
                    infoTemplate.friction.x = prop.value("value", 0.9f);
                    hasTileFrictionX = true;
                }
                else if (pName == "Friction_Y")
                {
                    infoTemplate.friction.y = prop.value("value", 0.9f);
                    hasTileFrictionY = true;
                }
                else if (pName == "Collision")
                {
                    const std::string val = prop.value("value", "Solid");
                    if (val == "OneWay") infoTemplate.collision = TileCollision::OneWay;
                    else if (val == "None") infoTemplate.collision = TileCollision::None;
                    else infoTemplate.collision = TileCollision::Solid;
                }
            }

            if (hasTileFrictionX && !hasTileFrictionY) infoTemplate.friction.y = infoTemplate.friction.x;
            if (!hasTileFrictionX && hasTileFrictionY) infoTemplate.friction.x = infoTemplate.friction.y;

            tileTemplates[id] = infoTemplate;
        }
    }
}

void MapTmjLoader::LoadLayers(
    Map& map,
    const nlohmann::json& mapData,
    const std::unordered_map<int, TileInfo>& tileTemplates,
    unsigned int tilesPerRow,
    const std::string& path)
{
    unsigned int playerObjectCount = 0u;
    unsigned int doorObjectCount = 0u;

    for (const auto& layer : mapData["layers"])
    {
        const std::string layerType = layer.value("type", "");

        if (layerType == "tilelayer")
        {
            ProcessTileLayer(map, layer, tileTemplates, tilesPerRow, path);
        }
        else if (layerType == "objectgroup")
        {
            ProcessObjectLayer(map, layer, path, playerObjectCount, doorObjectCount);
        }
    }

    if (playerObjectCount == 0u)
    {
        EngineLog::WarnOnce(
            "map.object.player.missing." + path,
            "No Player object found in map '" + path + "'. Respawn fallback will use (0,0).");
    }
}

void MapTmjLoader::Load(Map& map, const std::string& path)
{
    EngineLog::ResetOnce("map.tileset.external_tsx");
    EngineLog::ResetOnce("map.spawn.player.failed");
    EngineLog::ResetOnce("map.spawn.enemy.failed");

    nlohmann::json mapData;
    if (!LoadJson(path, mapData))
        return;

    ResetPerMapDefaults(map);

    if (!ReadMapGeometry(map, mapData, path))
        return;

    const unsigned int tilesPerRow = PrepareTilesetTexture(map);

    LoadMapPropertiesAndBackgrounds(map, mapData);

    std::unordered_map<int, TileInfo> tileTemplates;
    BuildTileTemplates(map, mapData, tileTemplates);

    if (!mapData.contains("layers") || !mapData["layers"].is_array())
        return;

    LoadLayers(map, mapData, tileTemplates, tilesPerRow, path);
    map.RefreshBackgroundScale();
}