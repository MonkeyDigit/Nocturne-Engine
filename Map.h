#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

class Map
{
public:
    Map();

    void Draw(sf::RenderWindow& window);
    bool IsSolid(int x, int y) const;

    static constexpr float TILE_SIZE = 32.0f;

private:
    std::vector<std::vector<int>> m_grid;
    sf::RectangleShape m_tileShape;
};