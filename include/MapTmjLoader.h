#pragma once

#include <string>
#include <unordered_map>
#include "json.hpp"
#include "Map.h"

class MapTmjLoader
{
public:
    static bool Load(Map& map, const std::string& path);

private:
    static std::string ToLowerCopy(std::string value);
    static std::string CanonicalObjectType(std::string rawType);

    static bool LoadJson(const std::string& path, nlohmann::json& outData);
    static void ResetPerMapDefaults(Map& map);
    static bool ReadMapGeometry(Map& map, const nlohmann::json& mapData, const std::string& path);
    static unsigned int PrepareTilesetTexture(Map& map);
    static void LoadMapPropertiesAndBackgrounds(Map& map, const nlohmann::json& mapData);

    static void BuildTileTemplates(
        Map& map,
        const nlohmann::json& mapData,
        std::unordered_map<int, TileInfo>& tileTemplates);

    static bool LoadLayers(
        Map& map,
        const nlohmann::json& mapData,
        const std::unordered_map<int, TileInfo>& tileTemplates,
        unsigned int tilesPerRow,
        const std::string& path);

    static bool ProcessTileLayer(
        Map& map,
        const nlohmann::json& layer,
        const std::unordered_map<int, TileInfo>& tileTemplates,
        unsigned int tilesPerRow,
        const std::string& path);

    static bool ProcessObjectLayer(
        Map& map,
        const nlohmann::json& layer,
        const std::string& path,
        unsigned int& playerObjectCount,
        unsigned int& doorObjectCount);

    // Object parsing helpers (member functions to keep friend access to Map internals)
    static std::string ResolveRawObjectType(const nlohmann::json& object);
    static bool TryReadFiniteObjectRect(
        const nlohmann::json& object,
        float& outX,
        float& outY,
        float& outW,
        float& outH);

    static void HandlePlayerObject(
        Map& map,
        const std::string& path,
        const std::string& name,
        float objX,
        float objY,
        float mapPixelWidth,
        float mapPixelHeight,
        unsigned int& playerObjectCount);

    static void HandleEnemyObject(
        Map& map,
        const std::string& path,
        const std::string& name,
        float objX,
        float objY);

    static void HandleDoorObject(
        Map& map,
        const std::string& path,
        float objX,
        float objY,
        float objW,
        float objH,
        unsigned int& doorObjectCount);

    static void HandleTrapObject(
        Map& map,
        const std::string& path,
        float objX,
        float objY,
        float objW,
        float objH);
};