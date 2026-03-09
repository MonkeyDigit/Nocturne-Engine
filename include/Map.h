#pragma once
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

struct SharedContext;
class BaseState;
class MapTmjLoader;

using TileID = unsigned int;

enum class TileCollision {
    None = 0,
    Solid,
    OneWay
};

struct TileInfo
{
    unsigned int id = 0;
    std::string name = "";
    sf::Vector2f friction = { 0.0f, 0.0f };
    bool deadly = false;
    sf::IntRect textureRect = {}; // Holds the exact crop coordinates from the tilesheet
    TileCollision collision = TileCollision::Solid;
};

struct Tile
{
    TileInfo properties;
    bool warp = false;
};

struct BackgroundLayer {
    BackgroundLayer(const sf::Texture& tex, const std::string& name)
        : sprite(tex), textureName(name)
    {}
    sf::Sprite sprite;
    std::string textureName;
};

using TileMap = std::unordered_map<TileID, std::unique_ptr<Tile>>;

class Map
{
public:
    Map(SharedContext& context, BaseState* currentState);
    ~Map();

    bool LoadMap(const std::string& path);
    void LoadNext();
    void Update();
    void Draw(sf::RenderWindow& window);

    Tile* GetTile(int x, int y) const;
    TileInfo* GetDefaultTile();
    float GetGravity() const;
    unsigned int GetTileSize() const { return m_tileWidth; }
    const sf::Vector2u& GetMapSize() const;
    const sf::Vector2f& GetPlayerStart() const;
    bool IsNextMapQueued() const { return m_loadNextMap; }

    const sf::FloatRect& GetDoorRect() const { return m_doorRect; }
    const std::vector<sf::FloatRect>& GetTraps() const { return m_traps; }

private:
    friend class MapTmjLoader;

    unsigned int ConvertCoords(unsigned int x, unsigned int y) const;
    void PurgeMap();
    void RefreshBackgroundScale();
    sf::Vector2f m_lastViewSize;

    int m_playerId;
    TileMap m_tileMap;

    sf::Texture* m_tileTexture;

    std::vector<sf::VertexArray> m_layerVertices;
    std::vector<BackgroundLayer> m_backgrounds;

    unsigned int m_tileWidth;
    unsigned int m_tileHeight;

    sf::Vector2u m_maxMapSize;
    sf::Vector2f m_playerStart;

    sf::FloatRect m_doorRect;
    std::vector<sf::FloatRect> m_traps;

    float m_mapGravity;
    std::string m_nextMap;
    bool m_loadNextMap;

    TileInfo m_defaultTile;

    SharedContext& m_context;
    BaseState* m_currentState;
};