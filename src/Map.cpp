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

Map::Map(SharedContext& context, BaseState* currentState)
    : m_context(context),
    m_currentState(currentState),
    m_maxMapSize({ 32, 32 }),
    m_mapGravity(512.0f),
    m_loadNextMap(false),
    m_tileTexture(nullptr),
    m_backgroundTexture(nullptr),
    m_playerId(-1)
{
    m_context.m_gameMap = this;

    m_defaultTile.friction = sf::Vector2f(0.9f, 0.9f);
    m_defaultTile.deadly = false;

    m_vertices.setPrimitiveType(sf::PrimitiveType::Triangles);
}

Map::~Map()
{
    PurgeMap();
    m_context.m_gameMap = nullptr;
}

unsigned int Map::ConvertCoords(unsigned int x, unsigned int y) const
{
    return (x * m_maxMapSize.x) + y;
}

Tile* Map::GetTile(unsigned int x, unsigned int y) const
{
    auto itr = m_tileMap.find(ConvertCoords(x, y));
    return (itr != m_tileMap.end() ? itr->second.get() : nullptr);
}

TileInfo* Map::GetDefaultTile() { return &m_defaultTile; }
float Map::GetGravity() const { return m_mapGravity; }
unsigned int Map::GetTileSize() const { return Sheet::TILE_SIZE; }
const sf::Vector2u& Map::GetMapSize() const { return m_maxMapSize; }
const sf::Vector2f& Map::GetPlayerStart() const { return m_playerStart; }

void Map::LoadMap(const std::string& path)
{
    std::ifstream file(Utils::GetWorkingDirectory() + path);
    if (!file.is_open())
    {
        std::cerr << "! Failed loading map file: " << path << '\n';
        return;
    }

    nlohmann::json mapData;
    try {
        file >> mapData;
    }
    catch (const nlohmann::json::parse_error& e) {
        std::cerr << "! JSON Parse Error: " << e.what() << '\n';
        return;
    }
    file.close();

    // Safely extract values with fallbacks to prevent crashes
    m_maxMapSize.x = mapData.value("width", 32);
    m_maxMapSize.y = mapData.value("height", 32);

    if (!mapData.contains("layers") || !mapData["layers"].is_array()) return;

    for (const auto& layer : mapData["layers"])
    {
        std::string layerType = layer.value("type", "");

        // ==========================================
        // --- TILE LAYER ---
        // ==========================================
        if (layerType == "tilelayer")
        {
            if (!layer.contains("data") || !layer["data"].is_array()) continue;

            const auto& data = layer["data"];
            unsigned int x = 0;
            unsigned int y = 0;

            // Calculate how many tiles fit in a single row of the texture sheet
            int tilesPerRow = Sheet::SHEET_WIDTH / Sheet::TILE_SIZE;

            for (int tileID : data)
            {
                if (tileID > 0)
                {
                    int actualID = tileID - 1;

                    auto tile = std::make_unique<Tile>();
                    tile->warp = false;

                    // Assign properties dynamically
                    tile->properties.id = actualID;
                    tile->properties.friction = m_defaultTile.friction;
                    tile->properties.deadly = false;

                    // The Magic Math: calculate crop coordinates without tiles.cfg!
                    tile->properties.textureRect = sf::IntRect(
                        { static_cast<int>((actualID % tilesPerRow) * Sheet::TILE_SIZE),
                          static_cast<int>((actualID / tilesPerRow) * Sheet::TILE_SIZE) },
                        { static_cast<int>(Sheet::TILE_SIZE), static_cast<int>(Sheet::TILE_SIZE) }
                    );

                    m_tileMap.emplace(ConvertCoords(x, y), std::move(tile));
                }

                x++;
                if (x >= m_maxMapSize.x) {
                    x = 0;
                    y++;
                }
            }
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
                std::string name = object.value("name", "Unknown");
                float objX = object.value("x", 0.0f);
                float objY = object.value("y", 0.0f);

                if (typeStr == "Player")
                {
                    if (m_playerId == -1) m_playerId = m_context.m_entityManager.Add(EntityType::Player, name);

                    m_playerStart = sf::Vector2f(objX, objY);

                    EntityBase* player = m_context.m_entityManager.Find(m_playerId);
                    if (player) player->SetPosition(m_playerStart);
                }
                else if (typeStr == "Enemy")
                {
                    int enemyId = m_context.m_entityManager.Add(EntityType::Enemy, name);
                    if (enemyId >= 0)
                    {
                        EntityBase* enemy = m_context.m_entityManager.Find(enemyId);
                        if (enemy) enemy->SetPosition(sf::Vector2f(objX, objY));
                    }
                }
            }
        }
    }

    BuildVertexArray();
}

void Map::BuildVertexArray()
{
    if (m_context.m_textureManager.RequireResource("MapTileSet"))
        m_tileTexture = m_context.m_textureManager.GetResource("MapTileSet");

    m_vertices.clear();
    m_vertices.resize(m_tileMap.size() * 6);
    int vertexIndex = 0;

    for (const auto& pair : m_tileMap)
    {
        unsigned int x = pair.first / m_maxMapSize.x;
        unsigned int y = pair.first % m_maxMapSize.x;

        // Note the '&' here! We are getting a pointer to the value stored in the struct
        TileInfo* info = &pair.second->properties;

        sf::Vector2f pos(static_cast<float>(x * Sheet::TILE_SIZE), static_cast<float>(y * Sheet::TILE_SIZE));
        float size = static_cast<float>(Sheet::TILE_SIZE);

        sf::FloatRect texCoords(
            { static_cast<float>(info->textureRect.position.x), static_cast<float>(info->textureRect.position.y) },
            { static_cast<float>(info->textureRect.size.x), static_cast<float>(info->textureRect.size.y) }
        );

        sf::Vertex* quad = &m_vertices[vertexIndex];

        quad[0].position = pos;
        quad[1].position = sf::Vector2f(pos.x + size, pos.y);
        quad[2].position = sf::Vector2f(pos.x, pos.y + size);

        quad[0].texCoords = sf::Vector2f(texCoords.position.x, texCoords.position.y);
        quad[1].texCoords = sf::Vector2f(texCoords.position.x + texCoords.size.x, texCoords.position.y);
        quad[2].texCoords = sf::Vector2f(texCoords.position.x, texCoords.position.y + texCoords.size.y);

        quad[3].position = sf::Vector2f(pos.x, pos.y + size);
        quad[4].position = sf::Vector2f(pos.x + size, pos.y);
        quad[5].position = sf::Vector2f(pos.x + size, pos.y + size);

        quad[3].texCoords = sf::Vector2f(texCoords.position.x, texCoords.position.y + texCoords.size.y);
        quad[4].texCoords = sf::Vector2f(texCoords.position.x + texCoords.size.x, texCoords.position.y);
        quad[5].texCoords = sf::Vector2f(texCoords.position.x + texCoords.size.x, texCoords.position.y + texCoords.size.y);

        vertexIndex += 6;
    }
}

void Map::LoadNext() { m_loadNextMap = true; }

void Map::Update(float deltaTime)
{
    if (m_loadNextMap)
    {
        PurgeMap();
        m_loadNextMap = false;
        if (!m_nextMap.empty()) LoadMap("media/maps/" + m_nextMap);
        m_nextMap = "";
    }

    sf::FloatRect viewSpace = m_context.m_window.GetViewSpace();
    if (m_background)
        m_background->setPosition({ viewSpace.position.x, viewSpace.position.y });
}

void Map::Draw(sf::RenderWindow& window)
{
    if (m_background) window.draw(*m_background);
    if (m_tileTexture)
    {
        sf::RenderStates states;
        states.texture = m_tileTexture;
        window.draw(m_vertices, states);
    }
}

void Map::PurgeMap()
{
    m_tileMap.clear();
    m_context.m_entityManager.Purge();
    m_playerId = -1;

    if (!m_backgroundTextureName.empty())
    {
        m_context.m_textureManager.ReleaseResource(m_backgroundTextureName);
        m_backgroundTextureName = "";
        m_backgroundTexture = nullptr;
        m_background.reset();
    }
}