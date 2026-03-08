#include <cmath>
#include <sstream>
#include <SFML/Graphics/RectangleShape.hpp>
#include "State_Game.h"
#include "StateManager.h"
#include "SharedContext.h"
#include "Window.h"
#include "EntityManager.h"
#include "CState.h"
#include "CSprite.h"
#include "EngineLog.h"
#include "CTransform.h"
#include "CBoxCollider.h"
#include "CombatGeometry.h"

State_Game::State_Game(StateManager& stateManager)
    : BaseState(stateManager),
    m_stillCursorTime(0.0f),
    m_cursorVisible(true),
    m_debugMode(false),
    m_gameMap(stateManager.GetContext(), this),
    m_playerIdCache(-1),
    m_fpsText(m_debugFont),
    m_fpsAccumTime(0.0f),
    m_fpsFrameCount(0),
    m_currentFps(0.0f),
    m_debugFontLoaded(false)
{}

State_Game::~State_Game() {}

void State_Game::OnCreate()
{
    EventManager& evMgr = m_stateManager.GetContext().m_eventManager;

    // Registering callbacks using the modern EventDetails reference
    evMgr.AddCallback(StateType::Game, "OpenMainMenu", &State_Game::MainMenu, *this);
    evMgr.AddCallback(StateType::Game, "Pause", &State_Game::Pause, *this);
    evMgr.AddCallback(StateType::Game, "ToggleDebug", &State_Game::ToggleDebugOverlay, *this);

    // Mouse logic
    m_stillCursorTime = 0.0f;
    m_mousePos = evMgr.GetMousePos(m_stateManager.GetContext().m_window.GetRenderWindow());
    m_cursorVisible = true;

    m_gameMap.LoadMap("media/maps/map_1.tmj");

    m_hud = std::make_unique<HUD>(m_stateManager.GetContext().GetEntityManager());

    m_playerIdCache = -1;

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

void State_Game::Update(const sf::Time& time)
{
    // Keep cursor visibility logic independent from gameplay update
    UpdateCursor(time);

    // Resolve player once per frame (fast ID path + fallback by name)
    EntityBase* player = ResolvePlayer();

    // If player exists, process death/transition hazards; otherwise try respawn
    if (player)
        HandlePlayerHazards(*player);
    else
        RespawnPlayer();

    // Core world update.
    SharedContext& context = m_stateManager.GetContext();
    context.GetEntityManager().Update(time.asSeconds());
    m_gameMap.Update();

    // UI overlay update (guarded for safety)
    if (m_hud)
        m_hud->Update();
}

void State_Game::Draw()
{
    SharedContext& context = m_stateManager.GetContext();
    sf::RenderWindow& window = context.m_window.GetRenderWindow();

    const sf::View gameView = ResolveGameView();
    ApplyGameView(window, gameView);

    m_gameMap.Draw(window);
    context.GetEntityManager().Draw();

    if (m_debugMode)
        DrawDebugOverlay(window, gameView);

    DrawHudOverlay(window, gameView);
}

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

    // Hide cursor after 3 seconds of inactivity
    if (m_stillCursorTime >= 3.0f && m_cursorVisible)
        SetCursorVisible(false);
}

void State_Game::SetCursorVisible(bool visible)
{
    m_cursorVisible = visible;
    m_stateManager.GetContext().m_window.GetRenderWindow().setMouseCursorVisible(m_cursorVisible);
}

void State_Game::InitializeDebugOverlay()
{
    m_debugFontLoaded = m_debugFont.openFromFile("media/fonts/EightBitDragon.ttf");

    if (!m_debugFontLoaded)
    {
        EngineLog::WarnOnce("font.debug.fps_failed", "Failed to load debug font for FPS counter");
    }
    else
    {
        m_fpsText.setCharacterSize(20);
        m_fpsText.setFillColor(sf::Color::White);
        m_fpsText.setOutlineColor(sf::Color::Black);
        m_fpsText.setOutlineThickness(1.0f);
        m_fpsText.setString("FPS: --");
    }

    ResetFpsCounter();
}

void State_Game::ResetFpsCounter()
{
    m_fpsClock.restart();
    m_fpsAccumTime = 0.0f;
    m_fpsFrameCount = 0;
    m_currentFps = 0.0f;

    if (m_debugFontLoaded)
        m_fpsText.setString("FPS: --");
}

void State_Game::DrawDebugOverlay(sf::RenderWindow& window, const sf::View& gameView)
{
    DrawDebugHitboxes(window);
    DrawFpsCounter(window, gameView);
}

void State_Game::DrawDebugHitboxes(sf::RenderWindow& window)
{
    SharedContext& context = m_stateManager.GetContext();

    for (auto& entityPair : context.GetEntityManager().GetEntities())
    {
        EntityBase* entity = entityPair.second.get();
        CBoxCollider* collider = entity->GetComponent<CBoxCollider>();
        CSprite* sprite = entity->GetComponent<CSprite>();
        CState* state = entity->GetComponent<CState>();

        if (!collider || !sprite) continue;

        sf::FloatRect bodyRect = collider->GetAABB();

        sf::RectangleShape bodyShape(bodyRect.size);
        bodyShape.setPosition(bodyRect.position);
        bodyShape.setFillColor(sf::Color::Transparent);
        bodyShape.setOutlineColor(sf::Color::Cyan);
        bodyShape.setOutlineThickness(1.0f);
        window.draw(bodyShape);

        const sf::FloatRect attackWorldRect = ComputeWorldAttackAABB(collider, sprite);
        if (attackWorldRect.size.x <= 0.0f || attackWorldRect.size.y <= 0.0f) continue;

        sf::RectangleShape attackShape({ attackWorldRect.size.x, attackWorldRect.size.y });
        attackShape.setPosition(attackWorldRect.position);
        attackShape.setFillColor(sf::Color::Transparent);

        const bool isAttacking = (state && state->GetState() == EntityState::Attacking);
        if (isAttacking)
        {
            attackShape.setOutlineColor(sf::Color::Red);
            attackShape.setOutlineThickness(2.0f);
        }
        else
        {
            attackShape.setOutlineColor(sf::Color::Yellow);
            attackShape.setOutlineThickness(1.0f);
        }

        window.draw(attackShape);
    }
}

void State_Game::DrawFpsCounter(sf::RenderWindow& window, const sf::View& gameView)
{
    if (!m_debugFontLoaded) return;

    const float dt = m_fpsClock.restart().asSeconds();
    m_fpsAccumTime += dt;
    ++m_fpsFrameCount;

    if (m_fpsAccumTime >= 0.25f)
    {
        m_currentFps = static_cast<float>(m_fpsFrameCount) / m_fpsAccumTime;
        m_fpsAccumTime = 0.0f;
        m_fpsFrameCount = 0;

        std::ostringstream ss;
        ss << "FPS: " << static_cast<int>(std::round(m_currentFps));
        m_fpsText.setString(ss.str());

        if (m_currentFps >= 55.0f)
            m_fpsText.setFillColor(sf::Color(80, 220, 120));
        else if (m_currentFps >= 30.0f)
            m_fpsText.setFillColor(sf::Color(240, 210, 80));
        else
            m_fpsText.setFillColor(sf::Color(230, 80, 80));
    }

    SharedContext& context = m_stateManager.GetContext();
    window.setView(context.m_window.GetUIView());

    const sf::Vector2f uiRes = context.m_window.GetUIResolution();
    const sf::FloatRect b = m_fpsText.getLocalBounds();
    m_fpsText.setOrigin({ b.position.x + b.size.x, b.position.y });
    m_fpsText.setPosition({ uiRes.x - 20.0f, 20.0f });

    window.draw(m_fpsText);
    window.setView(gameView);
}

EntityBase* State_Game::ResolvePlayer()
{
    EntityManager& entityManager = m_stateManager.GetContext().GetEntityManager();

    // Fast path: cached player ID
    if (m_playerIdCache >= 0)
    {
        if (EntityBase* cached = entityManager.Find(static_cast<unsigned int>(m_playerIdCache)))
            return cached;
    }

    // Slow path: fallback by name, then refresh cache
    EntityBase* player = entityManager.Find("Player");
    m_playerIdCache = player ? static_cast<int>(player->GetId()) : -1;
    return player;
}

EntityBase* State_Game::RespawnPlayer()
{
    SharedContext& context = m_stateManager.GetContext();
    EntityManager& entityManager = context.GetEntityManager();

    EngineLog::Info("Respawning player...");
    const int playerId = entityManager.Add(EntityType::Player, "Player");

    if (playerId < 0)
    {
        // Avoid log spam while keeping the issue visible
        EngineLog::ErrorOnce(
            "respawn.player.blocked",
            "Respawn blocked: player could not be created (entity limit or creation error)");
        m_playerIdCache = -1;
        return nullptr;
    }

    EngineLog::ResetOnce("respawn.player.blocked");
    m_playerIdCache = playerId;

    EntityBase* player = entityManager.Find(static_cast<unsigned int>(playerId));
    if (player)
    {
        if (CTransform* transform = player->GetComponent<CTransform>())
        {
            // Spawn at map-defined player start
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

    // Require all gameplay-critical components
    if (!transform || !collider || !state || state->GetState() == EntityState::Dying)
        return;

    const float mapHeight = static_cast<float>(m_gameMap.GetMapSize().y * m_gameMap.GetTileSize());
    const sf::FloatRect pBounds = collider->GetAABB();

    // Kill if player falls outside map height.
    if (transform->GetPosition().y >= mapHeight)
        state->InstantKill();

    // Queue next map when touching the door trigger
    if (m_gameMap.GetDoorRect().findIntersection(pBounds))
        m_gameMap.LoadNext();

    // Kill on trap collision
    for (const auto& trap : m_gameMap.GetTraps())
    {
        if (trap.findIntersection(pBounds))
        {
            state->InstantKill();
            break;
        }
    }
}

sf::View State_Game::ResolveGameView()
{
    SharedContext& context = m_stateManager.GetContext();
    EntityManager& entityManager = context.GetEntityManager();

    const sf::View baseGameView = context.m_window.GetGameView();
    sf::View gameView = entityManager.GetCameraSystem().GetCurrentView();

    // Keep a valid camera view even in edge cases
    if (gameView.getSize().x <= 0.0f || gameView.getSize().y <= 0.0f)
        gameView = baseGameView;

    // Keep viewport aligned with the configured game viewport
    gameView.setViewport(baseGameView.getViewport());
    return gameView;
}

void State_Game::ApplyGameView(sf::RenderWindow& window, const sf::View& gameView)
{
    window.setView(gameView);
}

void State_Game::DrawHudOverlay(sf::RenderWindow& window, const sf::View& gameView)
{
    if (!m_hud) return;

    SharedContext& context = m_stateManager.GetContext();
    window.setView(context.m_window.GetUIView());
    m_hud->Draw(window);

    // Restore gameplay view for any subsequent draw call
    window.setView(gameView);
}