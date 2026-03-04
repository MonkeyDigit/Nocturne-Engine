#include "Map.h"
#include "EntityManager.h"
#include "SharedContext.h"
#include "EntityBase.h"
#include "Utilities.h"
#include "StateManager.h"
#include "TextureManager.h"
#include "Window.h"
#include <fstream>
#include <sstream>
#include <iostream>

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

void Map::LoadTiles(const std::string& path)
{
    std::ifstream file{ Utils::GetWorkingDirectory() + path };
    if (!file.is_open()) return;

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

        tile->textureRect = sf::IntRect(
            { static_cast<int>((tileId % (Sheet::SHEET_WIDTH / Sheet::TILE_SIZE)) * Sheet::TILE_SIZE),
             static_cast<int>((tileId / (Sheet::SHEET_HEIGHT / Sheet::TILE_SIZE)) * Sheet::TILE_SIZE) },
            { Sheet::TILE_SIZE, Sheet::TILE_SIZE }
        );

        m_tileSet.emplace(tileId, std::move(tile));
    }
    file.close();
}

void Map::LoadMap(const std::string& path)
{
    std::ifstream file{ Utils::GetWorkingDirectory() + path };
    if (!file.is_open()) return;

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

            if (tileCoords.x >= static_cast<int>(m_maxMapSize.x) || tileCoords.y >= static_cast<int>(m_maxMapSize.y)) continue;

            auto tile = std::make_unique<Tile>();
            tile->properties = itr->second.get();
            tile->warp = false;

            if (!m_tileMap.emplace(ConvertCoords(tileCoords.x, tileCoords.y), std::move(tile)).second) continue;

            std::string warp;
            keystream >> warp;
            if (warp == "WARP") m_tileMap[ConvertCoords(tileCoords.x, tileCoords.y)]->warp = true;
        }
        else if (type == "BACKGROUND")
        {
            std::string bgName;
            keystream >> bgName;

            if (m_context.m_textureManager.RequireResource(bgName))
            {
                m_backgroundTextureName = bgName;
                m_backgroundTexture = m_context.m_textureManager.GetResource(bgName);
                m_background.emplace(*m_backgroundTexture);

                sf::Vector2f viewSize = m_currentState->GetView().getSize();
                sf::Vector2u textureSize = m_backgroundTexture->getSize();
                sf::Vector2f scaleFactors;
                scaleFactors.y = viewSize.y / textureSize.y;
                scaleFactors.x = scaleFactors.y;
                m_background->setScale(scaleFactors);
            }
        }
        else if (type == "SIZE") keystream >> m_maxMapSize.x >> m_maxMapSize.y;
        else if (type == "GRAVITY") keystream >> m_mapGravity;
        else if (type == "DEFAULT_FRICTION") keystream >> m_defaultTile.friction.x >> m_defaultTile.friction.y;
        else if (type == "NEXTMAP") keystream >> m_nextMap;
        else if (type == "PLAYER")
        {
            if (m_playerId == -1) m_playerId = m_context.m_entityManager.Add(EntityType::Player, "Player");

            keystream >> m_playerStart.x >> m_playerStart.y;
            EntityBase* player = m_context.m_entityManager.Find(m_playerId);
            if (player) player->SetPosition(m_playerStart);
        }
        else if (type == "ENEMY")
        {
            std::string enemyName;
            keystream >> enemyName;
            int enemyId = m_context.m_entityManager.Add(EntityType::Enemy, enemyName);
            if (enemyId < 0) continue;

            sf::Vector2f enemyPos;
            keystream >> enemyPos.x >> enemyPos.y;
            EntityBase* enemy = m_context.m_entityManager.Find(enemyId);
            if (enemy) enemy->SetPosition(enemyPos);
        }
    }
    file.close();

    BuildVertexArray();
}

void Map::BuildVertexArray()
{
    if (m_context.m_textureManager.RequireResource("TileSheet"))
    {
        m_tileTexture = m_context.m_textureManager.GetResource("TileSheet");
    }

    m_vertices.clear();
    m_vertices.resize(m_tileMap.size() * 6);
    int vertexIndex = 0;

    for (const auto& pair : m_tileMap)
    {
        unsigned int x = pair.first / m_maxMapSize.x;
        unsigned int y = pair.first % m_maxMapSize.x;

        TileInfo* info = pair.second->properties;

        sf::Vector2f pos(x * Sheet::TILE_SIZE, y * Sheet::TILE_SIZE);
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

void Map::PurgeTileSet() { m_tileSet.clear(); }