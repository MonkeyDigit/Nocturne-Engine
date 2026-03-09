#include <algorithm>
#include <cassert>
#include <cmath>

#include "Game.h"
#include "EngineLog.h"
#include "GameplayTuningLoader.h"

namespace
{
    constexpr float kDefaultFixedUpdateHz = 60.0f;
    constexpr float kMinFixedUpdateHz = 15.0f;
    constexpr float kMaxFixedUpdateHz = 240.0f;

    constexpr float kDefaultMaxFrameTimeSeconds = 0.25f;
    constexpr float kMinMaxFrameTimeSeconds = 0.01f;
    constexpr float kMaxMaxFrameTimeSeconds = 1.0f;

    constexpr unsigned int kDefaultMaxUpdatesPerFrame = 8u;
    constexpr unsigned int kMinMaxUpdatesPerFrame = 1u;
    constexpr unsigned int kMaxMaxUpdatesPerFrame = 120u;

    float SanitizeFixedUpdateHz(float value)
    {
        if (!std::isfinite(value) || value <= 0.0f)
            return kDefaultFixedUpdateHz;

        return std::clamp(value, kMinFixedUpdateHz, kMaxFixedUpdateHz);
    }

    float SanitizeMaxFrameTimeSeconds(float value)
    {
        if (!std::isfinite(value) || value <= 0.0f)
            return kDefaultMaxFrameTimeSeconds;

        return std::clamp(value, kMinMaxFrameTimeSeconds, kMaxMaxFrameTimeSeconds);
    }

    unsigned int SanitizeMaxUpdatesPerFrame(unsigned int value)
    {
        if (value == 0u)
            return kDefaultMaxUpdatesPerFrame;

        return std::clamp(value, kMinMaxUpdatesPerFrame, kMaxMaxUpdatesPerFrame);
    }
}

Game::Game()
    : m_window("Project Nocturne", { 1280, 720 }),
    m_textureManager(),
    m_audioManager(),

    // Initialize the context passing references to the core systems
    m_context(m_window, m_window.GetEventManager(), m_textureManager, m_audioManager),

    // Advanced systems are instantiated receiving the complete context
    m_entityManager(m_context, 100),
    m_stateManager(m_context)
{
    m_context.SetEntityManager(m_entityManager);
    assert(m_context.HasEntityManager());
    GameplayTuningLoader::Load("config/gameplay.cfg", m_context.m_gameplayTuning);

    if (m_window.HasFixedAISeed())
    {
        m_entityManager.SetAISeed(m_window.GetFixedAISeed());
        EngineLog::Info("AI RNG seed: " + std::to_string(m_window.GetFixedAISeed()) + " (fixed)");
    }
    else
    {
        EngineLog::Info("AI RNG seed: " + std::to_string(m_entityManager.GetAISeed()) + " (random)");
    }

    // Start from game intro
    m_stateManager.SwitchTo(StateType::Intro);
}

void Game::Run()
{
    const GameplayTuning& tuning = m_context.m_gameplayTuning;

    const float fixedHz = SanitizeFixedUpdateHz(tuning.m_fixedUpdateHz);
    const sf::Time timePerFrame = sf::seconds(1.0f / fixedHz);

    const float maxFrameSeconds = SanitizeMaxFrameTimeSeconds(tuning.m_maxFrameTimeSeconds);
    const sf::Time maxFrameTime = sf::seconds(maxFrameSeconds);

    const unsigned int maxUpdatesPerFrame = SanitizeMaxUpdatesPerFrame(tuning.m_maxUpdatesPerFrame);

    m_context.m_runtimeFrameStats.m_fixedStepSeconds = timePerFrame.asSeconds();
    m_context.m_runtimeFrameStats.m_maxUpdatesPerFrame = maxUpdatesPerFrame;
    m_context.m_runtimeFrameStats.m_lastFixedUpdates = 0u;
    m_context.m_runtimeFrameStats.m_droppedBacklog = false;

    sf::Clock clock;
    sf::Time timeSinceLastUpdate = sf::Time::Zero;

    while (!m_window.IsDone())
    {
        sf::Time elapsedTime = clock.restart();

        // Clamp very large frame times (e.g. debugger break, window drag, alt-tab spike)
        if (elapsedTime > maxFrameTime)
            elapsedTime = maxFrameTime;

        m_context.m_runtimeFrameStats.m_elapsedFrameSeconds = elapsedTime.asSeconds();
        timeSinceLastUpdate += elapsedTime;

        // Fixed timestep (accumulator pattern)
        unsigned int updates = 0;
        while (timeSinceLastUpdate >= timePerFrame && updates < maxUpdatesPerFrame)
        {
            timeSinceLastUpdate -= timePerFrame;
            Update(timePerFrame);
            ++updates;
        }

        const bool droppedBacklog =
            (updates == maxUpdatesPerFrame && timeSinceLastUpdate >= timePerFrame);

        // Drop residual backlog to avoid perpetual catch-up under heavy load
        if (droppedBacklog)
            timeSinceLastUpdate = sf::Time::Zero;

        m_context.m_runtimeFrameStats.m_lastFixedUpdates = updates;
        m_context.m_runtimeFrameStats.m_droppedBacklog = droppedBacklog;
        if (droppedBacklog)
            ++m_context.m_runtimeFrameStats.m_backlogDropCount;

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
    m_stateManager.ProcessRequests();
    m_context.m_audioManager.Update();
}

void Game::Render()
{
    m_window.BeginDraw();

    // The StateManager will handle drawing the menu background or the map + entities
    m_stateManager.Draw();

    m_window.EndDraw();
}