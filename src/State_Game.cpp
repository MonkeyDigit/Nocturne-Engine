#include "State_Game.h"
#include "StateManager.h"
#include "SharedContext.h"
#include "Window.h"
#include <iostream>
#include <cmath>

State_Game::State_Game(StateManager& stateManager)
    : BaseState(stateManager),
    m_stillCursorTime(0.0f),
    m_cursorVisible(true),
    m_enemy(250.0f, 64.0f),
    m_debugMode(false)
{
}

State_Game::~State_Game() {}

void State_Game::OnCreate()
{
    // Using half the resolution for a pixel-perfect 2x zoom look
    m_view.setSize({ 640.0f, 360.0f });
    m_view.setCenter(m_view.getSize() * 0.5f);
    AdjustView();

    EventManager& evMgr = m_stateManager.GetContext().m_eventManager;

    // Registering callbacks using the modern EventDetails reference
    // NOTE: Make sure "Pause" and "ToggleDebug" are in your Bindings.cfg!
    evMgr.AddCallback(StateType::Game, "Pause", &State_Game::Pause, *this);
    evMgr.AddCallback(StateType::Game, "ToggleDebug", &State_Game::ToggleOverlay, *this);

    // Mouse logic
    m_stillCursorTime = 0.0f;
    m_mousePos = evMgr.GetMousePos();
    m_cursorVisible = true;

    m_stateManager.GetContext().m_window.GetRenderWindow().setView(m_view);
}

void State_Game::OnDestroy()
{
    EventManager& evMgr = m_stateManager.GetContext().m_eventManager;
    evMgr.RemoveCallback(StateType::Game, "Pause");
    evMgr.RemoveCallback(StateType::Game, "ToggleDebug");
}

void State_Game::Activate() {}

void State_Game::Deactivate() {}

void State_Game::Update(const sf::Time& time)
{
    UpdateCursor(time);
    SharedContext& context = m_stateManager.GetContext();

    // 1. Update Entities (Using our Phase 6 classes temporarily)
    m_player.Update(time, m_gameMap, context.m_eventManager);
    m_enemy.Update(time, m_gameMap);

    // Hardcoded Combat check from Phase 6
    if (m_player.IsAttacking())
    {
        sf::FloatRect attackBounds = m_player.GetAttackBounds();
        if (attackBounds.findIntersection(m_enemy.GetBounds()))
        {
            m_enemy.TakeDamage(1, static_cast<float>(m_player.GetFacingDirection()));
        }
    }

    // 2. Camera Tracking (Keep player centered)
    sf::Vector2f playerPos = m_player.GetPosition();
    m_view.setCenter({ playerPos.x, playerPos.y });

    // Smooth Camera clamping to prevent seeing outside the map
    sf::Vector2f viewSize = m_view.getSize();
    float mapWidth = 30.0f * 16.0f;  // Phase 6 hardcoded map width
    float mapHeight = 30.0f * 16.0f; // Phase 6 hardcoded map height

    float viewX = std::clamp(m_view.getCenter().x, viewSize.x / 2.0f, mapWidth - viewSize.x / 2.0f);
    float viewY = std::clamp(m_view.getCenter().y, viewSize.y / 2.0f, mapHeight - viewSize.y / 2.0f);

    m_view.setCenter({ viewX, viewY });

    // Apply the updated view
    context.m_window.GetRenderWindow().setView(m_view);
}

void State_Game::Draw()
{
    SharedContext& context = m_stateManager.GetContext();
    sf::RenderWindow& window = context.m_window.GetRenderWindow();

    // Draw the Map first
    m_gameMap.Draw(window);

    // Draw Entities
    m_enemy.Draw(window);
    m_player.Draw(window);

    // Draw UI overlay here in the future
}

void State_Game::MainMenu(EventDetails& details)
{
    m_stillCursorTime = 0.0f;
    SetCursorVisible(true);
    m_stateManager.SwitchTo(StateType::MainMenu);
}

void State_Game::Pause(EventDetails& details)
{
    if (!details.m_heldDown)
    {
        m_stillCursorTime = 0.0f;
        SetCursorVisible(true);
        // Assuming you have a Paused state registered in StateManager
        m_stateManager.SwitchTo(StateType::Paused);
    }
}

void State_Game::ToggleOverlay(EventDetails& details)
{
    if (!details.m_heldDown)
    {
        m_debugMode = !m_debugMode;
        std::cout << "Debug Mode: " << (m_debugMode ? "ON" : "OFF") << "\n";
    }
}

void State_Game::UpdateCursor(const sf::Time& time)
{
    sf::Vector2i newMousePos = m_stateManager.GetContext().m_eventManager.GetMousePos();

    if (m_mousePos == newMousePos)
        m_stillCursorTime += time.asSeconds();
    else
    {
        m_stillCursorTime = 0.0f;
        if (!m_cursorVisible) SetCursorVisible(true);
    }

    m_mousePos = newMousePos;

    // Hide cursor after 3 seconds of inactivity
    if (m_stillCursorTime >= 3.0f && m_cursorVisible)
        SetCursorVisible(false);
}

void State_Game::SetCursorVisible(bool visible)
{
    m_cursorVisible = visible;
    // SFML 3 renamed setMouseCursorVisible to setMouseCursorVisible
    m_stateManager.GetContext().m_window.GetRenderWindow().setMouseCursorVisible(m_cursorVisible);
}