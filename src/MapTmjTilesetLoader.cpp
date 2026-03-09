#include "MapTmjLoader.h"
#include "SharedContext.h"
#include "TextureManager.h"
#include "EngineLog.h"

unsigned int MapTmjLoader::PrepareTilesetTexture(Map& map)
{
    // Release any past map tileset texture
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

    // Backward compatibility: if only one axis exists, mirror it
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