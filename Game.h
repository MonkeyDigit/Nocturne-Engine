#pragma once
#include <SFML/Graphics.hpp>

class Game {
public:
    Game();
    void Run();

private:
    void ProcessEvents();
    void Update(sf::Time deltaTime);
    void Render();

    sf::RenderWindow m_window;

    // Constant for 60 Hz fixed physics timestep (approx 0.0166667 seconds)
    const sf::Time TIME_PER_FRAME = sf::seconds(1.0f / 60.0f);
};