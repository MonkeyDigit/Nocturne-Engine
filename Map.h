#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class Map
{
public:
    Map();

    void Draw(sf::RenderWindow& window);
    bool IsSolid(int x, int y) const;

    static constexpr float TILE_SIZE = 32.0f;

    // Getters to know the real size of the world, useful for camera clamping
    float GetWidth() const;
    float GetHeight() const;

private:
    std::vector<std::string> m_grid;
    sf::RectangleShape m_tileShape;
};