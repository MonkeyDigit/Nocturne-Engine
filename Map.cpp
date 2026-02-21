#include <iostream> // For error logging
#include "Map.h"

Map::Map()
    : m_tileSprite(m_tileTexture)
{
    // A 50x30 map (1600x960 pixels, larger than our 1280x720 window)
    // '1' = Solid Wall, '0' = Empty Space
    // The design forces the player to fall down a shaft on the right,
    // fight the boss on the bottom left, and use Double Jump to climb back up.
    m_grid = {
        "11111111111111111111111111111111111111111111111111",
        "10000000000000000000000000000000000000000000000001",
        "10000000000000000000000000000000000000000000000001",
        "10000000000000000000000000000000000000000000000001",
        "10000000000000000000000000000000000000000000000001",
        "10000000000000000000000000000000000000000000000001",
        "10000000000000000000000000000000000000000000000001",
        "10000000000000000000000000000000000000000000000001",
        "10000000000000000000000000000000000000000000000001",
        "11111111111111111111111111111111111000001111111111", // Top floor with gap
        "10000000000000000000000000000000000000000000000001",
        "10000000000000000000000000000000000000000000000001",
        "10000000000000000000000000000000001111000000000001", // Very high platform
        "10000000000000000000000000000000000000000000000001",
        "10000000000000000000000000000000000000000000000001",
        "10000000000000000000000000000000000000001111000001", // Medium high platform
        "10000000000000000000000000000000000000000000000001",
        "10000000000000000000000000000000000000000000000001",
        "10000000000000000000000000000000001111000000000001", // Low platform
        "10000000000000000000000000000000000000000000000001",
        "10000000000000000000000000000000000000000000000001",
        "10000000000000000000000000000000000000000000000001",
        "10000000000000000000000000000000000000000000000001",
        "10000000000000000000000000000000000000000000000001",
        "10000000000000000000000000000000000000000000000001",
        "10000000000000000000000000000000000000000000000001",
        "10000000000000000000000000000000000000000000000001",
        "10000000000000000000000000000000000000000000000001",
        "10000000000000000000000000000000000000000000000001",
        "11111111111111111111111111111111111111111111111111"  // Bottom floor
    };

    // --- LOAD TEXTURE ---
    if (!m_tileTexture.loadFromFile("media/textures/TileSheet.png"))
    {
        std::cerr << "ERROR: Could not load TileSheet.png! Check working directory.\n" ;
    }

    // Define which part of the TileSheet to use. 
    // In SFML 3: sf::IntRect({posX, posY}, {width, height})
    // The first tile in the tile sheet is at top-left (0,0) and is 16x16
    m_tileSprite.setTextureRect(sf::IntRect({ 0, 0 }, { 16, 16 }));
}

bool Map::IsSolid(int x, int y) const
{
    // If we are out of bounds, consider everything solid not to make the player fall out
    if (y < 0 || y >= m_grid.size() || x < 0 || x >= m_grid[0].size()) return true;
    return m_grid[y][x] == '1'; // Char comparison
}

float Map::GetWidth() const { return m_grid[0].size() * TILE_SIZE; }
float Map::GetHeight() const { return m_grid.size() * TILE_SIZE; }

void Map::Draw(sf::RenderWindow& window)
{
    for (size_t y = 0; y < m_grid.size(); ++y)
    {
        for (size_t x = 0; x < m_grid[y].size(); ++x)
        {
            if (m_grid[y][x] == '1')
            {
                // Move the sprite to the grid coordinate and draw it
                m_tileSprite.setPosition({ x * TILE_SIZE, y * TILE_SIZE });
                window.draw(m_tileSprite);
            }
        }
    }
}