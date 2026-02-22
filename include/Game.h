#pragma once
#include <SFML/Graphics.hpp>
#include "Window.h"
#include "TextureManager.h"
#include "EntityManager.h"
#include "StateManager.h"
#include "SharedContext.h"

class Game
{
public:
    Game();
    ~Game() = default;

    void Run();

private:
    void Update(sf::Time deltaTime);
    void Render();

    // --- CORE ENGINE SYSTEMS ---
    // WARNING: The declaration order is CRITICAL in C++
    // Objects will be instantiated in memory in this exact order
    Window m_window;
    TextureManager m_textureManager;

    // The Context stores references to the systems above (and below)
    // Passing m_entityManager here is safe because it only stores the memory address
    SharedContext m_context;

    EntityManager m_entityManager;
    StateManager m_stateManager;
    // TODO: Missing debug overlay

    // Constant for a fixed 60 FPS physics simulation (Accumulator Pattern)
    const sf::Time TIME_PER_FRAME = sf::seconds(1.0f / 60.0f);
};