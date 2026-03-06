#include "Game.h"

Game::Game()
    : m_window("Project Nocturne", { 1280, 720 }),
    // m_textureManager default constructor is called implicitly

    // Initialize the context passing references to the core systems
    m_context(m_window, m_window.GetEventManager(), m_textureManager, m_entityManager, m_audioManager),

    // Advanced systems are instantiated receiving the complete context
    m_entityManager(m_context, 100),
    m_stateManager(m_context)
{
    // Start from game intro
    m_stateManager.SwitchTo(StateType::Intro);
}

void Game::Run()
{
    sf::Clock clock;
    sf::Time timeSinceLastUpdate = sf::Time::Zero;

    while (!m_window.IsDone())
    {
        sf::Time elapsedTime = clock.restart();
        timeSinceLastUpdate += elapsedTime;

        // FIXED TIMESTEP (Accumulator Pattern)
        // Guarantees that the physics simulation advances at a constant rate (60 FPS) 
        // regardless of the user's PC hardware power.
        while (timeSinceLastUpdate > TIME_PER_FRAME)
        {
            timeSinceLastUpdate -= TIME_PER_FRAME;
            Update(TIME_PER_FRAME);
        }

        // Render as fast as possible
        Render();
    }
}

void Game::Update(sf::Time deltaTime)
{
    // Process Window and OS events
    m_window.Update();

    // Delegate the update logic to the State Machine
    // (It will update the EntityManager if the current state is State_Game)
    m_stateManager.Update(deltaTime);
    // TODO: Spostare da qua?
    m_context.m_audioManager.Update();
}

void Game::Render()
{
    m_window.BeginDraw();

    // The StateManager will handle drawing the menu background or the map + entities
    m_stateManager.Draw();

    m_window.EndDraw();
}

// TODO: Aggiungere late update?