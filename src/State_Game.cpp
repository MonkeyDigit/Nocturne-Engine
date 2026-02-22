#include "State_Game.h"
#include "StateManager.h"
#include "SharedContext.h"
#include "Window.h"
#include "EntityManager.h"
#include <iostream>
#include <cmath>

State_Game::State_Game(StateManager& stateManager)
    : BaseState(stateManager),
    m_stillCursorTime(0.0f),
    m_cursorVisible(true),
    m_debugMode(false),
    m_gameMap(stateManager.GetContext(), this)
{}

State_Game::~State_Game() {}

void State_Game::OnCreate()
{
    // Using half the resolution for a pixel-perfect 2x zoom look
    m_view.setSize({ 640.0f, 360.0f });
    m_view.setCenter(m_view.getSize() * 0.5f);
    AdjustView();

    EventManager& evMgr = m_stateManager.GetContext().m_eventManager;

    // Registering callbacks using the modern EventDetails reference
    // TODO: Make sure "Pause" and "ToggleDebug" are in your Bindings.cfg
    evMgr.AddCallback(StateType::Game, "Pause", &State_Game::Pause, *this);
    evMgr.AddCallback(StateType::Game, "ToggleDebug", &State_Game::ToggleOverlay, *this);

    // Mouse logic
    m_stillCursorTime = 0.0f;
    m_mousePos = evMgr.GetMousePos();
    m_cursorVisible = true;

    m_stateManager.GetContext().m_window.GetRenderWindow().setView(m_view);

    m_gameMap.LoadMap("media/maps/map1.map");
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

    // Update Entities
    context.m_entityManager.Update(time.asSeconds());

    // Camera Tracking
    Character* player = static_cast<Character*>(context.m_entityManager.Find("Player"));
    if (player)
    {
        sf::Vector2f playerPos = player->GetPosition();
        m_view.setCenter({ playerPos.x, playerPos.y });
    }

    // ... [Smooth Camera clamping come prima] ...
    context.m_window.GetRenderWindow().setView(m_view);
}

void State_Game::Draw()
{
    SharedContext& context = m_stateManager.GetContext();
    sf::RenderWindow& window = context.m_window.GetRenderWindow();

    // Draw the Map first
    m_gameMap.Draw(window);

    // Draw Entities
    context.m_entityManager.Draw();

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
    m_stateManager.GetContext().m_window.GetRenderWindow().setMouseCursorVisible(m_cursorVisible);
}