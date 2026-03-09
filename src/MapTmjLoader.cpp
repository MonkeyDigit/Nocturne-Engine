#include <algorithm>
#include <cctype>
#include <fstream>

#include "MapTmjLoader.h"
#include "Utilities.h"
#include "EngineLog.h"

namespace
{
    bool TryReadPositiveUnsignedField(const nlohmann::json& jsonData, const char* key, unsigned int& outValue)
    {
        if (!jsonData.contains(key) || !jsonData[key].is_number_unsigned())
            return false;

        outValue = jsonData[key].get<unsigned int>();
        return outValue > 0u;
    }

    unsigned int CountLayersOfType(const nlohmann::json& mapData, const std::string& type)
    {
        if (!mapData.contains("layers") || !mapData["layers"].is_array())
            return 0u;

        unsigned int count = 0u;
        for (const auto& layer : mapData["layers"])
        {
            if (layer.value("type", "") == type)
                ++count;
        }

        return count;
    }
}

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
        EngineLog::Error(std::string("JSON Parse Error in map '") + path + "': " + e.what());
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
    unsigned int width = 0u;
    unsigned int height = 0u;
    unsigned int tileWidth = 0u;
    unsigned int tileHeight = 0u;

    if (!TryReadPositiveUnsignedField(mapData, "width", width))
    {
        EngineLog::Error("Invalid or missing 'width' in map '" + path + "' (expected unsigned > 0).");
        return false;
    }

    if (!TryReadPositiveUnsignedField(mapData, "height", height))
    {
        EngineLog::Error("Invalid or missing 'height' in map '" + path + "' (expected unsigned > 0).");
        return false;
    }

    if (!TryReadPositiveUnsignedField(mapData, "tilewidth", tileWidth))
    {
        EngineLog::Error("Invalid or missing 'tilewidth' in map '" + path + "' (expected unsigned > 0).");
        return false;
    }

    if (!TryReadPositiveUnsignedField(mapData, "tileheight", tileHeight))
    {
        EngineLog::Error("Invalid or missing 'tileheight' in map '" + path + "' (expected unsigned > 0).");
        return false;
    }

    map.m_maxMapSize = { width, height };
    map.m_tileWidth = tileWidth;
    map.m_tileHeight = tileHeight;
    return true;
}

bool MapTmjLoader::LoadLayers(
    Map& map,
    const nlohmann::json& mapData,
    const std::unordered_map<int, TileInfo>& tileTemplates,
    unsigned int tilesPerRow,
    const std::string& path)
{
    unsigned int playerObjectCount = 0u;
    unsigned int doorObjectCount = 0u;
    unsigned int tileLayerCount = 0u;
    unsigned int objectLayerCount = 0u;

    for (const auto& layer : mapData["layers"])
    {
        const std::string layerType = layer.value("type", "");
        const std::string layerName = layer.value("name", "<unnamed>");

        if (layerType == "tilelayer")
        {
            ++tileLayerCount;
            if (!ProcessTileLayer(map, layer, tileTemplates, tilesPerRow, path))
                return false;
        }
        else if (layerType == "objectgroup")
        {
            ++objectLayerCount;
            if (!ProcessObjectLayer(map, layer, path, playerObjectCount, doorObjectCount))
                return false;
        }
        else
        {
            EngineLog::WarnOnce(
                "map.layer.unknown." + path + "." + layerName,
                "Unsupported layer type '" + layerType + "' in map '" + path +
                "' (layer '" + layerName + "'). Layer ignored.");
        }
    }

    if (tileLayerCount == 0u)
    {
        EngineLog::Error("Map '" + path + "' has no tile layers. Aborting load.");
        return false;
    }

    if (objectLayerCount == 0u)
    {
        EngineLog::WarnOnce(
            "map.object_layer.missing." + path,
            "Map '" + path + "' has no object layer. Gameplay entities may be missing.");
    }

    if (playerObjectCount == 0u)
    {
        EngineLog::Error("Map '" + path + "' has no Player object. Aborting load.");
        return false;
    }

    if (playerObjectCount > 1u)
    {
        EngineLog::Error("Map '" + path + "' has multiple Player objects. Aborting load.");
        return false;
    }

    return true;
}

bool MapTmjLoader::Load(Map& map, const std::string& path)
{
    EngineLog::ResetOnce("map.tileset.external_tsx");
    EngineLog::ResetOnce("map.spawn.player.failed");
    EngineLog::ResetOnce("map.spawn.enemy.failed");

    nlohmann::json mapData;
    if (!LoadJson(path, mapData))
        return false;

    ResetPerMapDefaults(map);

    if (!ReadMapGeometry(map, mapData, path))
        return false;

    if (mapData.value("infinite", false))
    {
        EngineLog::Error(
            "Map '" + path +
            "' uses infinite/chunked layers. This loader supports only finite maps.");
        return false;
    }

    const std::string orientation = mapData.value("orientation", "orthogonal");
    if (orientation != "orthogonal")
    {
        EngineLog::Error(
            "Map '" + path + "' has unsupported orientation '" + orientation +
            "'. Expected 'orthogonal'.");
        return false;
    }

    if (!mapData.contains("tilesets") || !mapData["tilesets"].is_array() || mapData["tilesets"].empty())
    {
        EngineLog::Error("Map '" + path + "' has no valid 'tilesets' array.");
        return false;
    }

    if (!mapData.contains("layers") || !mapData["layers"].is_array() || mapData["layers"].empty())
    {
        EngineLog::Error("Map '" + path + "' has no valid 'layers' array.");
        return false;
    }

    if (CountLayersOfType(mapData, "tilelayer") == 0u)
    {
        EngineLog::Error("Map '" + path + "' has zero tile layers.");
        return false;
    }

    const unsigned int tilesPerRow = PrepareTilesetTexture(map);

    LoadMapPropertiesAndBackgrounds(map, mapData);

    std::unordered_map<int, TileInfo> tileTemplates;
    BuildTileTemplates(map, mapData, tileTemplates);

    if (!LoadLayers(map, mapData, tileTemplates, tilesPerRow, path))
        return false;

    map.RefreshBackgroundScale();
    return true;
}