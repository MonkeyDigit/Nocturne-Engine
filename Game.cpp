#include "Game.h"

Game::Game()
// SFML 3 uses braced initialization for sf::VideoMode
    : m_window(sf::VideoMode({ 1280, 720 }), "Project Nocturne")
{
    // Disable VSync to test the robustness of our fixed timestep loop
    m_window.setVerticalSyncEnabled(false);
}

void Game::Run()
{
    sf::Clock clock;
    sf::Time timeSinceLastUpdate = sf::Time::Zero;

    while (m_window.isOpen())
    {
        // Measure the time passed since the last loop iteration
        sf::Time elapsedTime = clock.restart();
        timeSinceLastUpdate += elapsedTime;

        // ACCUMULATOR PATTERN:
        // Consume the accumulated time in discrete 1/60s chunks
        // This ensures the physics run at a constant rate regardless of frame rate
        while (timeSinceLastUpdate > TIME_PER_FRAME)
        {
            timeSinceLastUpdate -= TIME_PER_FRAME;

            ProcessEvents();
            // We ALWAYS pass the fixed constant to the Update method
            Update(TIME_PER_FRAME);
        }

        // Render as fast as possible
        Render();
    }
}

void Game::ProcessEvents()
{
    // SFML 3 uses std::optional for event polling
    while (const std::optional event = m_window.pollEvent())
    {
        // Type-safe event checking using templates
        if (event->is<sf::Event::Closed>())
        {
            m_window.close();
        }
        // TODO: Future input bindings (Key pressed/released) will go here
    }
}

void Game::Update(sf::Time deltaTime)
{
    // TODO: Here we will update our mathematical Player logic later
}

void Game::Render()
{
    m_window.clear(sf::Color(30, 30, 30));

    // TODO: Drawing of map and entities will happen here

    m_window.display();
}