#include "State_Game.h"
#include "StateManager.h"
#include "SharedContext.h"
#include "Window.h"
#include "EventManager.h"
#include "EntityManager.h"
#include "CTransform.h"
#include "CBoxCollider.h"
#include "CState.h"
#include "EngineLog.h"

namespace
{
    constexpr float kCursorHideDelaySeconds = 3.0f;
}

void State_Game::OnCreate()
{
    EventManager& evMgr = m_stateManager.GetContext().m_eventManager;

    evMgr.AddCallback(StateType::Game, "OpenMainMenu", &State_Game::MainMenu, *this);
    evMgr.AddCallback(StateType::Game, "Pause", &State_Game::Pause, *this);
    evMgr.AddCallback(StateType::Game, "ToggleDebug", &State_Game::ToggleDebugOverlay, *this);

    m_stillCursorTime = 0.0f;
    m_mousePos = evMgr.GetMousePos(m_stateManager.GetContext().m_window.GetRenderWindow());
    m_cursorVisible = true;

    if (!m_gameMap.LoadMap("media/maps/map_1.tmj"))
    {
        EngineLog::Error("State_Game initialization failed: map load failed.");
        m_stateManager.SwitchTo(StateType::MainMenu);
        return;
    }

    m_hud = std::make_unique<HUD>(m_stateManager.GetContext().GetEntityManager());

    InitializeDebugOverlay();
}

void State_Game::OnDestroy()
{
    EventManager& evMgr = m_stateManager.GetContext().m_eventManager;
    evMgr.RemoveCallback(StateType::Game, "OpenMainMenu");
    evMgr.RemoveCallback(StateType::Game, "Pause");
    evMgr.RemoveCallback(StateType::Game, "ToggleDebug");
}

void State_Game::Activate() {}
void State_Game::Deactivate() {}

void State_Game::MainMenu(EventDetails&)
{
    m_stillCursorTime = 0.0f;
    SetCursorVisible(true);
    m_stateManager.SwitchTo(StateType::MainMenu);
    m_stateManager.Remove(StateType::Game);
}

void State_Game::Pause(EventDetails&)
{
    m_stillCursorTime = 0.0f;
    SetCursorVisible(true);
    m_stateManager.SwitchTo(StateType::Paused);
}

void State_Game::ToggleDebugOverlay(EventDetails&)
{
    m_debugMode = !m_debugMode;
    EngineLog::Info(std::string("Debug Mode: ") + (m_debugMode ? "ON" : "OFF"));

    if (m_debugMode)
        ResetFpsCounter();
}

void State_Game::UpdateCursor(const sf::Time& time)
{
    sf::RenderWindow& window = m_stateManager.GetContext().m_window.GetRenderWindow();
    sf::Vector2i newMousePos = m_stateManager.GetContext().m_eventManager.GetMousePos(window);

    if (m_mousePos == newMousePos)
        m_stillCursorTime += time.asSeconds();
    else
    {
        m_stillCursorTime = 0.0f;
        if (!m_cursorVisible) SetCursorVisible(true);
    }

    m_mousePos = newMousePos;

    if (m_stillCursorTime >= kCursorHideDelaySeconds && m_cursorVisible)
        SetCursorVisible(false);
}

void State_Game::SetCursorVisible(bool visible)
{
    m_cursorVisible = visible;
    m_stateManager.GetContext().m_window.GetRenderWindow().setMouseCursorVisible(m_cursorVisible);
}

EntityBase* State_Game::ResolvePlayer()
{
    return m_stateManager.GetContext().GetEntityManager().GetPlayer();
}

EntityBase* State_Game::RespawnPlayer()
{
    SharedContext& context = m_stateManager.GetContext();
    EntityManager& entityManager = context.GetEntityManager();

    EngineLog::Info("Respawning player...");
    const int playerId = entityManager.Add(EntityType::Player, "Player");

    if (playerId < 0)
    {
        EngineLog::ErrorOnce(
            "respawn.player.blocked",
            "Respawn blocked: player could not be created (entity limit or creation error)");
        return nullptr;
    }

    EngineLog::ResetOnce("respawn.player.blocked");

    EntityBase* player = entityManager.Find(static_cast<unsigned int>(playerId));
    if (player)
    {
        if (CTransform* transform = player->GetComponent<CTransform>())
        {
            transform->SetPosition(m_gameMap.GetPlayerStart());
        }
    }

    return player;
}

void State_Game::HandlePlayerHazards(EntityBase& player)
{
    CTransform* transform = player.GetComponent<CTransform>();
    CBoxCollider* collider = player.GetComponent<CBoxCollider>();
    CState* state = player.GetComponent<CState>();

    if (!transform || !collider || !state || state->GetState() == EntityState::Dying)
        return;

    const float mapHeight = static_cast<float>(m_gameMap.GetMapSize().y * m_gameMap.GetTileSize());
    const sf::FloatRect pBounds = collider->GetAABB();

    if (transform->GetPosition().y >= mapHeight)
    {
        state->InstantKill();
        return;
    }

    for (const auto& trap : m_gameMap.GetTraps())
    {
        if (trap.findIntersection(pBounds))
        {
            state->InstantKill();
            return;
        }
    }

    if (m_gameMap.GetDoorRect().findIntersection(pBounds))
    {
        if (m_gameMap.HasNextMap())
        {
            m_gameMap.LoadNext();
        }
        else
        {
            // End-of-slice condition: no next map configured, go to victory state
            m_stillCursorTime = 0.0f;
            SetCursorVisible(true);
            m_stateManager.SwitchTo(StateType::Victory);
            m_stateManager.Remove(StateType::Game);
        }

        return;
    }
}