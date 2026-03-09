#include <fstream>
#include <algorithm>
#include <cctype>
#include <cmath>
#include "json.hpp"
#include "Map.h"
#include "EntityManager.h"
#include "SharedContext.h"
#include "EntityBase.h"
#include "Utilities.h"
#include "TextureManager.h"
#include "EngineLog.h"

namespace
{
    std::string ToLowerCopy(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return value;
    }

    std::string CanonicalObjectType(std::string rawType)
    {
        rawType = ToLowerCopy(std::move(rawType));

        if (rawType == "player" || rawType == "spawn_player") return "player";
        if (rawType == "enemy") return "enemy";
        if (rawType == "door" || rawType == "exit") return "door";
        if (rawType == "trap" || rawType == "hazard") return "trap";

        return rawType;
    }
}

void Map::LoadMap(const std::string& path)
{
    EngineLog::ResetOnce("map.tileset.external_tsx");
    EngineLog::ResetOnce("map.spawn.player.failed");
    EngineLog::ResetOnce("map.spawn.enemy.failed");

    std::ifstream file(Utils::GetWorkingDirectory() + path);
    if (!file.is_open())
    {
        EngineLog::Error("Failed loading map file: " + path);
        return;
    }

    nlohmann::json mapData;
    try { file >> mapData; }
    catch (const nlohmann::json::parse_error& e)
    {
        EngineLog::Error(std::string("JSON Parse Error: ") + e.what());
        return;
    }
    file.close();

    // Reset per-map defaults to avoid carrying values from previous maps
    m_mapGravity = 512.0f;
    m_defaultTile.friction = { 0.9f, 0.9f };
    m_nextMap.clear();
    m_playerStart = { 0.0f, 0.0f };
    m_doorRect = sf::FloatRect();
    m_traps.clear();

    // Safely extract values with fallbacks to prevent crashes
    m_maxMapSize.x = mapData.value("width", 32u);
    m_maxMapSize.y = mapData.value("height", 32u);

    // Read tile size from JSON and guard invalid values
    m_tileWidth = mapData.value("tilewidth", 16u);
    m_tileHeight = mapData.value("tileheight", 16u);

    if (m_tileWidth == 0u || m_tileHeight == 0u)
    {
        EngineLog::Error("Invalid map tile size in '" + path + "' (tilewidth/tileheight must be > 0).");
        return;
    }

    // Release any past textures
    if (m_tileTexture)
    {
        m_context.m_textureManager.ReleaseResource("MapTileSet");
        m_tileTexture = nullptr;
    }

    // Load the tileset texture immediately to calculate crop coordinates
    if (m_context.m_textureManager.RequireResource("MapTileSet"))
        m_tileTexture = m_context.m_textureManager.GetResource("MapTileSet");

    unsigned int tilesPerRow = 1u;

    if (!m_tileTexture)
    {
        EngineLog::WarnOnce("map.tileset.missing", "Map tileset texture 'MapTileSet' is missing or failed to load.");
    }
    else
    {
        const unsigned int textureWidth = m_tileTexture->getSize().x;
        if (textureWidth < m_tileWidth)
        {
            EngineLog::WarnOnce(
                "map.tileset.too_narrow",
                "Tileset texture width is smaller than tile width. Falling back to one column.");
        }
        else
        {
            tilesPerRow = textureWidth / m_tileWidth;
            if (tilesPerRow == 0u) tilesPerRow = 1u;
        }
    }

    // LOADING PROPERTIES AND BACKGROUNDS
    bool hasMapFrictionX = false;
    bool hasMapFrictionY = false;
    if (mapData.contains("properties") && mapData["properties"].is_array())
    {
        for (const auto& prop : mapData["properties"])
        {
            std::string propName = prop.value("name", "");
            if (propName == "Gravity") m_mapGravity = prop.value("value", 512.0f);
            else if (propName == "NextMap") m_nextMap = prop.value("value", "");
            else if (propName == "Friction_X")
            {
                m_defaultTile.friction.x = prop.value("value", 0.9f);
                hasMapFrictionX = true;
            }
            else if (propName == "Friction_Y")
            {
                m_defaultTile.friction.y = prop.value("value", 0.9f);
                hasMapFrictionY = true;
            }
            // Load backgrounds
            else if (propName.find("Bg_") == 0)
            {
                std::string texName = prop.value("value", "");
                if (texName.empty())
                {
                    EngineLog::Warn("Map background property has empty texture id.");
                    continue;
                }

                if (!m_context.m_textureManager.RequireResource(texName))
                {
                    EngineLog::WarnOnce(
                        "map.bg.require_failed." + texName,
                        "Failed to require background texture '" + texName + "'");
                    continue;
                }

                sf::Texture* tex = m_context.m_textureManager.GetResource(texName);
                if (!tex)
                {
                    EngineLog::WarnOnce(
                        "map.bg.null_after_require." + texName,
                        "Background texture '" + texName + "' is null after successful require.");
                    continue;
                }

                m_backgrounds.emplace_back(*tex, texName);  // emplace_back calls the custom BackgroundLayer constructor
            }
        }

        // Keep backward compatibility: if only one axis is provided, mirror it
        if (hasMapFrictionX && !hasMapFrictionY) m_defaultTile.friction.y = m_defaultTile.friction.x;
        if (!hasMapFrictionX && hasMapFrictionY) m_defaultTile.friction.x = m_defaultTile.friction.y;
    }

    // ==========================================
    // --- READING TILE PROPERTIES ---
    // ==========================================
    // Temporary map to store properties
    std::unordered_map<int, TileInfo> tileTemplates;

    if (mapData.contains("tilesets") && mapData["tilesets"].is_array())
    {
        for (const auto& tileset : mapData["tilesets"])
        {
            // Give warning if not embedded
            if (tileset.contains("source") && tileset["source"].get<std::string>().find(".tsx") != std::string::npos)
                EngineLog::WarnOnce("map.tileset.external_tsx", "Tileset is external (.tsx). Please EMBED it in Tiled to read tile properties.");

            // Read tile properties
            if (tileset.contains("tiles") && tileset["tiles"].is_array())
            {
                for (const auto& tile : tileset["tiles"])
                {
                    int id = tile.value("id", -1);
                    if (id >= 0 && tile.contains("properties") && tile["properties"].is_array())
                    {
                        // Create base model for tile
                        TileInfo infoTemplate;
                        infoTemplate.id = id;
                        infoTemplate.friction = m_defaultTile.friction;
                        infoTemplate.deadly = false;
                        infoTemplate.collision = TileCollision::Solid;

                        // Read each custom property
                        bool hasTileFrictionX = false;
                        bool hasTileFrictionY = false;
                        for (const auto& prop : tile["properties"])
                        {
                            std::string pName = prop.value("name", "");

                            if (pName == "Deadly") infoTemplate.deadly = prop.value("value", false);
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
                                std::string val = prop.value("value", "Solid");
                                if (val == "OneWay") infoTemplate.collision = TileCollision::OneWay;
                                else if (val == "None") infoTemplate.collision = TileCollision::None;
                                else infoTemplate.collision = TileCollision::Solid;
                            }
                        }

                        // Keep backward compatibility for old maps using only Friction_X.
                        if (hasTileFrictionX && !hasTileFrictionY) infoTemplate.friction.y = infoTemplate.friction.x;
                        if (!hasTileFrictionX && hasTileFrictionY) infoTemplate.friction.x = infoTemplate.friction.y;

                        // Save model in our dictionary
                        tileTemplates[id] = infoTemplate;
                    }
                }
            }
        }
    }

    // ==========================================
    // --- LAYER GENERATION ---
    // ==========================================
    if (!mapData.contains("layers") || !mapData["layers"].is_array()) return;

    unsigned int playerObjectCount = 0u;
    unsigned int doorObjectCount = 0u;

    for (const auto& layer : mapData["layers"])
    {
        std::string layerType = layer.value("type", "");

        if (layerType == "tilelayer")
        {
            if (!layer.contains("data") || !layer["data"].is_array()) continue;

            // Does the layer generate collisions or is it only decor?
            bool layerHasCollisions = true;
            if (layer.contains("properties"))
            {
                for (const auto& prop : layer["properties"])
                {
                    if (prop.value("name", "") == "HasCollisions")
                    {
                        layerHasCollisions = prop.value("value", true);
                    }
                }
            }

            sf::VertexArray vertexArray(sf::PrimitiveType::Triangles);
            const auto& data = layer["data"];

            const std::size_t expectedTiles =
                static_cast<std::size_t>(m_maxMapSize.x) * static_cast<std::size_t>(m_maxMapSize.y);

            if (data.size() != expectedTiles)
            {
                EngineLog::Warn("Tile layer size mismatch: expected " +
                    std::to_string(expectedTiles) + ", got " + std::to_string(data.size()) + ".");
            }

            // Read only the safe overlap between declared map size and provided data size
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
                const unsigned int x = static_cast<unsigned int>(i % m_maxMapSize.x);
                const unsigned int y = static_cast<unsigned int>(i / m_maxMapSize.x);

                // Generate graphics
                sf::Vector2f pos(static_cast<float>(x * m_tileWidth), static_cast<float>(y * m_tileHeight));
                sf::Vector2f size(static_cast<float>(m_tileWidth), static_cast<float>(m_tileHeight));
                sf::Vector2f texPos(static_cast<float>((actualID % static_cast<int>(tilesPerRow)) * m_tileWidth),
                    static_cast<float>((actualID / static_cast<int>(tilesPerRow)) * m_tileHeight));

                // Append the 6 vertices required to draw a quad (two triangles)
                vertexArray.append(sf::Vertex(pos, sf::Color::White, texPos));
                vertexArray.append(sf::Vertex({ pos.x + size.x, pos.y }, sf::Color::White, { texPos.x + size.x, texPos.y }));
                vertexArray.append(sf::Vertex({ pos.x, pos.y + size.y }, sf::Color::White, { texPos.x, texPos.y + size.y }));
                vertexArray.append(sf::Vertex({ pos.x, pos.y + size.y }, sf::Color::White, { texPos.x, texPos.y + size.y }));
                vertexArray.append(sf::Vertex({ pos.x + size.x, pos.y }, sf::Color::White, { texPos.x + size.x, texPos.y }));
                vertexArray.append(sf::Vertex({ pos.x + size.x, pos.y + size.y }, sf::Color::White, { texPos.x + size.x, texPos.y + size.y }));

                // Start from defaults, then override from tile template if present
                TileInfo currentTileInfo;
                currentTileInfo.id = static_cast<unsigned int>(actualID);
                currentTileInfo.friction = m_defaultTile.friction;
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

                    m_tileMap.emplace(ConvertCoords(x, y), std::move(tile));
                }
            }

            if (invalidTileTypeCount > 0u)
            {
                EngineLog::Warn(
                    "Layer '" + layerName + "' in map '" + path + "' has " +
                    std::to_string(invalidTileTypeCount) +
                    " non-integer tile entries (ignored).");
            }

            // Store the rendered geometry for this layer
            m_layerVertices.push_back(vertexArray);
        }
        // ==========================================
        // --- OBJECT LAYER ---
        // ==========================================
        else if (layerType == "objectgroup")
        {
            if (!layer.contains("objects") || !layer["objects"].is_array()) continue;

            const float mapPixelWidth = static_cast<float>(m_maxMapSize.x * m_tileWidth);
            const float mapPixelHeight = static_cast<float>(m_maxMapSize.y * m_tileHeight);

            for (const auto& object : layer["objects"])
            {
                // Support both new Tiled ("class") and old Tiled ("type")
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

                // Defensive guard against malformed JSON numeric values
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

                    m_playerStart = sf::Vector2f(objX, objY);

                    if (objX < 0.0f || objY < 0.0f || objX > mapPixelWidth || objY > mapPixelHeight)
                    {
                        EngineLog::Warn("Player spawn is outside map bounds in '" + path + "'.");
                    }

                    if (m_playerId == -1)
                    {
                        m_playerId = m_context.GetEntityManager().Add(EntityType::Player, name);
                        if (m_playerId < 0)
                        {
                            EngineLog::WarnOnce("map.spawn.player.failed", "Failed to spawn player from map object");
                            continue;
                        }
                    }

                    EntityBase* player = m_context.GetEntityManager().Find(static_cast<unsigned int>(m_playerId));
                    if (player)
                        player->SetPosition(m_playerStart);
                }
                else if (objectType == "enemy")
                {
                    if (name.empty() || name == "Unknown")
                    {
                        EngineLog::Warn("Enemy object missing type id in name field in '" + path + "'. Object skipped.");
                        continue;
                    }

                    int enemyId = m_context.GetEntityManager().Add(EntityType::Enemy, name);
                    if (enemyId < 0)
                    {
                        EngineLog::WarnOnce("map.spawn.enemy.failed", "Failed to spawn enemy from map object");
                        continue;
                    }

                    EntityBase* enemy = m_context.GetEntityManager().Find(static_cast<unsigned int>(enemyId));
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

                    m_doorRect = { {objX, objY}, {objW, objH} };
                }
                else if (objectType == "trap")
                {
                    if (objW <= 0.0f || objH <= 0.0f)
                    {
                        EngineLog::Warn("Trap object with non-positive size in '" + path + "'. Object skipped.");
                        continue;
                    }

                    m_traps.push_back({ {objX, objY}, {objW, objH} });
                }
                else if (!objectType.empty())
                {
                    EngineLog::WarnOnce(
                        "map.object.unknown_type." + path + "." + objectType,
                        "Unknown object type '" + objectType + "' in map '" + path + "'.");
                }
            }
        }
    }

    if (playerObjectCount == 0u)
    {
        EngineLog::WarnOnce(
            "map.object.player.missing." + path,
            "No Player object found in map '" + path + "'. Respawn fallback will use (0,0).");
    }

    RefreshBackgroundScale();
}