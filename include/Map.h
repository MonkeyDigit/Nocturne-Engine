#pragma once
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <optional>

class SharedContext;
class BaseState;

// TODO: Modificare?
namespace Sheet
{
    constexpr unsigned int TILE_SIZE = 16;
    constexpr unsigned int SHEET_WIDTH = 128;
    constexpr unsigned int SHEET_HEIGHT = 128;
}

using TileID = unsigned int;

struct TileInfo
{
    unsigned int id;
    std::string name;
    sf::Vector2f friction;
    bool deadly;
    sf::IntRect textureRect; // Holds the exact crop coordinates from the tilesheet
};

struct Tile
{
    TileInfo* properties; // Pointer to the shared TileInfo (owned by the TileSet)
    bool warp;
};

using TileMap = std::unordered_map<TileID, std::unique_ptr<Tile>>;
using TileSet = std::unordered_map<TileID, std::unique_ptr<TileInfo>>;

class Map
{
public:
    Map(SharedContext& context, BaseState* currentState);
    ~Map();

    void LoadMap(const std::string& path);
    void LoadNext();
    void Update(float deltaTime);
    void Draw(sf::RenderWindow& window);

    Tile* GetTile(unsigned int x, unsigned int y) const;
    TileInfo* GetDefaultTile();
    float GetGravity() const;
    unsigned int GetTileSize() const;
    const sf::Vector2u& GetMapSize() const;
    const sf::Vector2f& GetPlayerStart() const;

private:
    unsigned int ConvertCoords(unsigned int x, unsigned int y) const;
    void LoadTiles(const std::string& path);
    void PurgeMap();
    void PurgeTileSet();

    // Bakes the map into a single GPU call
    void BuildVertexArray();

    int m_playerId;
    TileSet m_tileSet;
    TileMap m_tileMap;

    sf::Texture* m_tileTexture;
    sf::VertexArray m_vertices; // Single draw call geometry

    // Using std::optional to delay the sprite creation
    std::optional<sf::Sprite> m_background;
    sf::Texture* m_backgroundTexture;
    std::string m_backgroundTextureName;

    sf::Vector2u m_maxMapSize;
    sf::Vector2f m_playerStart;

    float m_mapGravity;
    std::string m_nextMap;
    bool m_loadNextMap;

    TileInfo m_defaultTile;

    SharedContext& m_context;
    BaseState* m_currentState;
};