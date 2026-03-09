#include "Map.h"
#include "EntityManager.h"
#include "SharedContext.h"
#include "TextureManager.h"

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

void Map::LoadNext()
{
    // Queue transition only once
    if (!m_loadNextMap)
        m_loadNextMap = true;
}

void Map::Update()
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