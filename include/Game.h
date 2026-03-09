#pragma once
#include <SFML/Graphics.hpp>
#include "Window.h"
#include "SharedContext.h"
#include "TextureManager.h"
#include "EntityManager.h"
#include "StateManager.h"
#include "AudioManager.h"

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
    AudioManager m_audioManager;

    // The Context stores references to the systems above (and below)
    // Passing m_entityManager here is safe because it only stores the memory address
    SharedContext m_context;

    EntityManager m_entityManager;
    StateManager m_stateManager;
};