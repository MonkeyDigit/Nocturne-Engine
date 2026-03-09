#include <algorithm>
#include <cctype>
#include <fstream>

#include "MapTmjLoader.h"
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
    // Reset per-map defaults to avoid carrying values from previous maps
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