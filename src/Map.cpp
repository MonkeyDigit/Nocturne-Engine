#include <fstream>
#include <sstream>
#include <iostream>
#include "Map.h"
#include "Utilities.h"
#include "SharedContext.h"
#include "EntityManager.h"
#include "TextureManager.h"
#include "Window.h"
#include "EntityBase.h"

Map::Map(SharedContext& context, BaseState* currentState)
    : m_context(context),
    m_currentState(currentState),
    m_maxMapSize({ 32, 32 }),
    m_mapGravity(512.0f),
    m_loadNextMap(false),
    m_tileTexture(nullptr),
    m_backgroundTexture(nullptr)
{
    m_context.m_gameMap = this;

    // Initialize default tile
    m_defaultTile.friction = sf::Vector2f(0.9f, 0.9f);
    m_defaultTile.deadly = false;

    // SFML 3: Use Triangles for drawing simple textured quads
    m_vertices.setPrimitiveType(sf::PrimitiveType::Triangles);

    LoadTiles("config/tiles.cfg");
}

Map::~Map()
{
    PurgeMap();
    PurgeTileSet();
    m_context.m_gameMap = nullptr;
}

unsigned int Map::ConvertCoords(unsigned int x, unsigned int y) const
{
    return (x * m_maxMapSize.x) + y; // Row-major mapping
}

Tile* Map::GetTile(unsigned int x, unsigned int y) const
{
    auto itr = m_tileMap.find(ConvertCoords(x, y));
    return (itr != m_tileMap.end() ? itr->second.get() : nullptr);
}

bool Map::IsSolid(unsigned int x, unsigned int y) const
{
    // If the coordinates are out of bounds, treat them as invisible solid walls
    if (x < 0 || x >= m_maxMapSize.x || y < 0 || y >= m_maxMapSize.y) return true;

    return GetTile(x, y) != nullptr;
}

float Map::GetGravity() const { return m_mapGravity; }
unsigned int Map::GetTileSize() const { return Sheet::TILE_SIZE; }
const sf::Vector2u& Map::GetMapSize() const { return m_maxMapSize; }
const sf::Vector2f& Map::GetPlayerStart() const { return m_playerStart; }

void Map::LoadTiles(const std::string& path)
{
    std::ifstream file{ Utils::GetWorkingDirectory() + path };
    if (!file.is_open())
    {
        std::cerr << "! Failed loading tile set file: " << path << '\n';
        return;
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '|') continue;

        std::stringstream keystream(line);
        int tileId;
        keystream >> tileId;
        if (tileId < 0) continue;

        auto tile = std::make_unique<TileInfo>();
        tile->id = tileId;

        keystream >> tile->name >> tile->friction.x >> tile->friction.y >> tile->deadly;

        // Calculate texture coordinates on the spritesheet
        tile->textureRect = sf::IntRect(
            { static_cast<int>((tileId % (Sheet::SHEET_WIDTH / Sheet::TILE_SIZE)) * Sheet::TILE_SIZE),
             static_cast<int>((tileId / (Sheet::SHEET_HEIGHT / Sheet::TILE_SIZE)) * Sheet::TILE_SIZE) },
            { Sheet::TILE_SIZE, Sheet::TILE_SIZE }
        );

        if (!m_tileSet.emplace(tileId, std::move(tile)).second)
        {
            std::cerr << "! Duplicate tile type: " << tileId << '\n';
        }
    }
    file.close();
}

void Map::LoadMap(const std::string& path)
{
    std::ifstream file{ Utils::GetWorkingDirectory() + path };
    if (!file.is_open())
    {
        std::cerr << "! Failed loading map: " << path << '\n';
        return;
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '|') continue;

        std::stringstream keystream(line);
        std::string type;
        keystream >> type;

        if (type == "TILE")
        {
            int tileId = 0;
            keystream >> tileId;
            if (tileId < 0) continue;

            auto itr = m_tileSet.find(tileId);
            if (itr == m_tileSet.end()) continue;

            sf::Vector2i tileCoords;
            keystream >> tileCoords.x >> tileCoords.y;

            if (tileCoords.x >= static_cast<int>(m_maxMapSize.x) || tileCoords.y >= static_cast<int>(m_maxMapSize.y))
            {
                continue; // Out of bounds
            }

            auto tile = std::make_unique<Tile>();
            tile->properties = itr->second.get();
            tile->warp = false;

            m_tileMap.emplace(ConvertCoords(tileCoords.x, tileCoords.y), std::move(tile));
        }
        else if (type == "BACKGROUND")
        {
            std::string bgName;
            keystream >> bgName;

            // TextureManager returns a raw pointer we can observe
            if (m_context.m_textureManager.RequireResource(bgName))
            {
                m_backgroundTextureName = bgName;
                m_backgroundTexture = m_context.m_textureManager.GetResource(bgName);
                m_background.emplace(*m_backgroundTexture);
            }
        }
        else if (type == "SIZE")
        {
            keystream >> m_maxMapSize.x >> m_maxMapSize.y;
        }
        else if (type == "GRAVITY")
        {
            keystream >> m_mapGravity;
        }
        else if (type == "DEFAULT_FRICTION")
        {
            keystream >> m_defaultTile.friction.x >> m_defaultTile.friction.y;
        }
        else if (type == "NEXTMAP")
        {
            keystream >> m_nextMap;
        }
        else if (type == "PLAYER")
        {
            keystream >> m_playerStart.x >> m_playerStart.y;
            // Spawning the player using the EntityManager
            int id = m_context.m_entityManager.Add(EntityType::Player, "Player");
            EntityBase* player = m_context.m_entityManager.Find(id);
            if (player) player->SetPosition(m_playerStart);
        }
        else if (type == "ENEMY")
        {
            std::string enemyName;
            sf::Vector2f enemyPos;
            keystream >> enemyName >> enemyPos.x >> enemyPos.y;

            // Spawning the enemy
            int id = m_context.m_entityManager.Add(EntityType::Enemy, enemyName);
            EntityBase* enemy = m_context.m_entityManager.Find(id);
            if (enemy) enemy->SetPosition(enemyPos);
        }
    }
    file.close();

    // After loading everything, we bake the map geometry for fast rendering
    BuildVertexArray();
}

void Map::BuildVertexArray()
{
    // Request the global TileSheet texture
    if (m_context.m_textureManager.RequireResource("TileSheet"))
    {
        m_tileTexture = m_context.m_textureManager.GetResource("TileSheet");
    }

    // 6 vertices per tile (2 triangles to make a square)
    m_vertices.clear();
    m_vertices.resize(m_tileMap.size() * 6);

    int vertexIndex = 0;

    for (const auto& pair : m_tileMap)
    {
        unsigned int x = pair.first / m_maxMapSize.x;
        unsigned int y = pair.first % m_maxMapSize.x;

        TileInfo* info = pair.second->properties;

        // The position of the tile in the world
        sf::Vector2f pos(x * Sheet::TILE_SIZE, y * Sheet::TILE_SIZE);
        float size = static_cast<float>(Sheet::TILE_SIZE);

        // The coordinates of the tile on the spritesheet (SFML 3 style)
        sf::FloatRect texCoords(
            sf::Vector2f(info->textureRect.position.x, info->textureRect.position.y),
            sf::Vector2f(info->textureRect.size.x, info->textureRect.size.y)
        );

        // Build the two triangles (6 vertices)
        sf::Vertex* quad = &m_vertices[vertexIndex];

        // Triangle 1
        quad[0].position = pos;
        quad[1].position = sf::Vector2f(pos.x + size, pos.y);
        quad[2].position = sf::Vector2f(pos.x, pos.y + size);

        quad[0].texCoords = sf::Vector2f(texCoords.position.x, texCoords.position.y);
        quad[1].texCoords = sf::Vector2f(texCoords.position.x + texCoords.size.x, texCoords.position.y);
        quad[2].texCoords = sf::Vector2f(texCoords.position.x, texCoords.position.y + texCoords.size.y);

        // Triangle 2
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
    // Draw background
    if (m_background)
        window.draw(*m_background);

    // Draw the ENTIRE map in one single GPU call. Extremely fast
    if (m_tileTexture)
    {
        sf::RenderStates states;
        states.texture = m_tileTexture;
        window.draw(m_vertices, states);
    }
}

void Map::PurgeMap()
{
    m_tileMap.clear(); // std::unique_ptr deletes automatically
    m_context.m_entityManager.Purge();

    if (!m_backgroundTextureName.empty())
    {
        m_context.m_textureManager.ReleaseResource(m_backgroundTextureName);
        m_backgroundTextureName = "";
        m_backgroundTexture = nullptr;
        m_background.reset();           // Clear safely
    }
}

void Map::PurgeTileSet()
{
    m_tileSet.clear(); // std::unique_ptr deletes automatically
}