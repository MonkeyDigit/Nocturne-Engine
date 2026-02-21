#pragma once
#include <SFML/Graphics.hpp>
#include "Player.h"
#include "Enemy.h"
#include "Boss.h"
#include "Window.h"

class Game {
public:
    Game();
    void Run();

private:
    void Update(sf::Time deltaTime);
    void Render();

    Window m_window;
    sf::View m_camera;
    Map m_map;
    Player m_player;
    Enemy m_enemy;
    Boss m_boss;
    bool m_bossDefeatedProcessed; // Per non chiamare "Unlock" ad ogni frame

    // Constant for 60 Hz fixed physics timestep (approx 0.0166667 seconds)
    const sf::Time TIME_PER_FRAME = sf::seconds(1.0f / 60.0f);
};