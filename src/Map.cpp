#include <fstream>
#include <iostream>
#include "json.hpp" // JSON parser
#include "Map.h"
#include "EntityManager.h"
#include "SharedContext.h"
#include "EntityBase.h"
#include "Utilities.h"
#include "StateManager.h"
#include "TextureManager.h"
#include "Window.h"
#include "EngineLog.h"

Map::Map(SharedContext& context, BaseState* currentState)
    : m_context(context),
    m_currentState(currentState),
    m_maxMapSize({ 32, 32 }),
    m_tileWidth(16),  // Default, will be overwritten by JSON
    m_tileHeight(16), // Default, will be overwritten by JSON
    m_mapGravity(512.0f),
    m_nextMap(""),
    m_loadNextMap(false),
    m_tileTexture(nullptr),
    m_playerId(-1),
    m_lastViewSize(0.0f, 0.0f)
{
    m_context.m_gameMap = this;

    m_defaultTile.friction = sf::Vector2f(0.9f, 0.9f);
    m_defaultTile.deadly = false;
}

Map::~Map()
{
    PurgeMap();
    m_context.m_gameMap = nullptr;
}

unsigned int Map::ConvertCoords(unsigned int x, unsigned int y) const
{
    return (y * m_maxMapSize.x) + x;
}

Tile* Map::GetTile(int x, int y) const
{
    if (x < 0 || y < 0) return nullptr;
    if (x >= static_cast<int>(m_maxMapSize.x) || y >= static_cast<int>(m_maxMapSize.y)) return nullptr;

    auto itr = m_tileMap.find(ConvertCoords(static_cast<unsigned int>(x), static_cast<unsigned int>(y)));
    return (itr != m_tileMap.end() ? itr->second.get() : nullptr);
}

TileInfo* Map::GetDefaultTile() { return &m_defaultTile; }
float Map::GetGravity() const { return m_mapGravity; }
const sf::Vector2u& Map::GetMapSize() const { return m_maxMapSize; }
const sf::Vector2f& Map::GetPlayerStart() const { return m_playerStart; }

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

    // Safely extract values with fallbacks to prevent crashes
    m_maxMapSize.x = mapData.value("width", 32);
    m_maxMapSize.y = mapData.value("height", 32);
    // Dynamically read tile sizes from the JSON file
    m_tileWidth = mapData.value("tilewidth", 16);
    m_tileHeight = mapData.value("tileheight", 16);

    // Release any past textures
    if (m_tileTexture)
    {
        m_context.m_textureManager.ReleaseResource("MapTileSet");
        m_tileTexture = nullptr;
    }

    // Load the tileset texture immediately to calculate crop coordinates
    if (m_context.m_textureManager.RequireResource("MapTileSet"))
        m_tileTexture = m_context.m_textureManager.GetResource("MapTileSet");

    int tilesPerRow = 1; // Prevent division by zero
    if (m_tileTexture && m_tileWidth > 0)
        tilesPerRow = m_tileTexture->getSize().x / m_tileWidth; // Calculate how many tiles fit in a single row of the texture sheet

    // LOADING PROPERTIES AND BACKGROUNDS
    if (mapData.contains("properties") && mapData["properties"].is_array())
    {
        for (const auto& prop : mapData["properties"])
        {
            std::string propName = prop.value("name", "");
            if (propName == "Gravity") m_mapGravity = prop.value("value", 512.0f);
            else if (propName == "NextMap") m_nextMap = prop.value("value", "");
            // TODO: Friction va messa come proprietà dei blocchi
            else if (propName == "Friction_X") m_defaultTile.friction.x = prop.value("value", 0.9f);
            // Load backgrounds
            else if (propName.find("Bg_") == 0)
            {
                std::string texName = prop.value("value", "");
                m_context.m_textureManager.RequireResource(texName);
                sf::Texture* tex = m_context.m_textureManager.GetResource(texName);
                if (tex) m_backgrounds.emplace_back(*tex, texName);  // emplace_back calls the custom BackgroundLayer constructor
            }
        }
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
                        for (const auto& prop : tile["properties"])
                        {
                            std::string pName = prop.value("name", "");

                            if (pName == "Deadly") infoTemplate.deadly = prop.value("value", false);
                            else if (pName == "Friction_X")
                            {
                                infoTemplate.friction.x = prop.value("value", 0.9f);
                                infoTemplate.friction.y = infoTemplate.friction.x;
                            }
                            else if (pName == "Collision")
                            {
                                std::string val = prop.value("value", "Solid");
                                if (val == "OneWay") infoTemplate.collision = TileCollision::OneWay;
                                else if (val == "None") infoTemplate.collision = TileCollision::None;
                                else infoTemplate.collision = TileCollision::Solid;
                            }
                        }
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
            unsigned int x = 0;
            unsigned int y = 0;

            for (int tileID : data)
            {
                if (tileID > 0)
                {
                    int actualID = tileID - 1;

                    // Generate graphics
                    sf::Vector2f pos(static_cast<float>(x * m_tileWidth), static_cast<float>(y * m_tileHeight));
                    sf::Vector2f size(static_cast<float>(m_tileWidth), static_cast<float>(m_tileHeight));
                    sf::Vector2f texPos(static_cast<float>((actualID % tilesPerRow) * m_tileWidth),
                        static_cast<float>((actualID / tilesPerRow) * m_tileHeight));

                    // Append the 6 vertices required to draw a quad (two triangles)
                    vertexArray.append(sf::Vertex(pos, sf::Color::White, texPos));
                    vertexArray.append(sf::Vertex({ pos.x + size.x, pos.y }, sf::Color::White, { texPos.x + size.x, texPos.y }));
                    vertexArray.append(sf::Vertex({ pos.x, pos.y + size.y }, sf::Color::White, { texPos.x, texPos.y + size.y }));

                    vertexArray.append(sf::Vertex({ pos.x, pos.y + size.y }, sf::Color::White, { texPos.x, texPos.y + size.y }));
                    vertexArray.append(sf::Vertex({ pos.x + size.x, pos.y }, sf::Color::White, { texPos.x + size.x, texPos.y }));
                    vertexArray.append(sf::Vertex({ pos.x + size.x, pos.y + size.y }, sf::Color::White, { texPos.x + size.x, texPos.y + size.y }));

                    // ==========================================
                    // --- APPLY TILE PROPERTIES ---
                    // ==========================================
                    // Start from a default set of properties
                    TileInfo currentTileInfo;
                    currentTileInfo.id = actualID;
                    currentTileInfo.friction = m_defaultTile.friction;
                    currentTileInfo.deadly = false;
                    currentTileInfo.collision = TileCollision::Solid;

                    if (tileTemplates.find(actualID) != tileTemplates.end())
                        currentTileInfo = tileTemplates[actualID];

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

                x++;
                if (x >= m_maxMapSize.x)
                {
                    x = 0;
                    y++;
                }
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

                std::string name = object.value("name", "Unknown");
                float objX = object.value("x", 0.0f);
                float objY = object.value("y", 0.0f);
                float objW = object.value("width", 32.0f);
                float objH = object.value("height", 32.0f);

                if (typeStr == "Player")
                {
                    m_playerStart = sf::Vector2f(objX, objY);

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
                else if (typeStr == "Enemy")
                {
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
                else if (typeStr == "Door") {
                    m_doorRect = { {objX, objY}, {objW, objH} };
                }
                else if (typeStr == "Trap") {
                    m_traps.push_back({ {objX, objY}, {objW, objH} });
                }
            }
        }
    }

    RefreshBackgroundScale();
}

void Map::LoadNext()
{
    // Queue transition only once
    if (!m_loadNextMap)
        m_loadNextMap = true;
}

void Map::Update(float deltaTime)
{
    if (m_loadNextMap)
    {
        PurgeMap();
        m_loadNextMap = false;
        if (!m_nextMap.empty()) LoadMap("media/maps/" + m_nextMap);
        m_nextMap = "";
    }

    // Fixed background position
    const sf::View& camView = m_context.GetEntityManager().GetCameraSystem().GetCurrentView();
    sf::FloatRect viewSpace(
        { camView.getCenter().x - (camView.getSize().x * 0.5f), camView.getCenter().y - (camView.getSize().y * 0.5f) },
        { camView.getSize().x, camView.getSize().y }
    );

    // Recompute scale only when view size changes
    if (camView.getSize() != m_lastViewSize)
        RefreshBackgroundScale();

    for (auto& bg : m_backgrounds)
    {
        bg.sprite.setPosition({ viewSpace.position.x, viewSpace.position.y });
    }
}

void Map::Draw(sf::RenderWindow& window)
{
    for (const auto& bg : m_backgrounds)
        window.draw(bg.sprite);

    if (m_tileTexture)
    {
        sf::RenderStates states;
        states.texture = m_tileTexture;

        for (const auto& layer : m_layerVertices)
            window.draw(layer, states);
    }
}

void Map::PurgeMap()
{
    m_tileMap.clear();
    m_layerVertices.clear();

    m_context.GetEntityManager().Purge();
    m_playerId = -1;

    if (m_tileTexture)
    {
        m_context.m_textureManager.ReleaseResource("MapTileSet");
        m_tileTexture = nullptr;
    }

    // Safely release textures for all backgrounds
    for (auto& bg : m_backgrounds) {
        m_context.m_textureManager.ReleaseResource(bg.textureName);
    }
    m_backgrounds.clear();

    m_doorRect = sf::FloatRect();
    m_traps.clear();
}

void Map::RefreshBackgroundScale()
{
    const sf::View& camView = m_context.GetEntityManager().GetCameraSystem().GetCurrentView();
    const sf::Vector2f viewSize = camView.getSize();

    // Skip if view size is invalid
    if (viewSize.x <= 0.0f || viewSize.y <= 0.0f) return;

    for (auto& bg : m_backgrounds)
    {
        sf::Vector2u texSize = bg.sprite.getTexture().getSize();
        if (texSize.x == 0 || texSize.y == 0) continue;

        bg.sprite.setScale({ viewSize.x / texSize.x, viewSize.y / texSize.y });
    }

    m_lastViewSize = viewSize;
}