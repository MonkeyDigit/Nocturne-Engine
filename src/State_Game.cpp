#include <iostream>
#include <cmath>
#include <SFML/Graphics/RectangleShape.hpp>
#include "State_Game.h"
#include "StateManager.h"
#include "SharedContext.h"
#include "Window.h"
#include "EntityManager.h"
#include "CState.h"
#include "CSprite.h"

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
    // TODO: Make sure "Pause" and "ToggleDebug" are in your Bindings.cfg
    // TODO: OPEN MENU
    evMgr.AddCallback(StateType::Game, "Pause", &State_Game::Pause, *this);
    evMgr.AddCallback(StateType::Game, "ToggleDebug", &State_Game::ToggleDebugOverlay, *this);

    // Mouse logic
    m_stillCursorTime = 0.0f;
    m_mousePos = evMgr.GetMousePos();
    m_cursorVisible = true;

    m_gameMap.LoadMap("media/maps/map_1.tmj");

    m_hud = std::make_unique<HUD>(m_stateManager.GetContext().GetEntityManager());

    m_playerIdCache = -1;

    // FPS Counter
    m_debugFontLoaded = m_debugFont.openFromFile("media/fonts/EightBitDragon.ttf");
    if (m_debugFontLoaded)
    {
        m_fpsText.setCharacterSize(20);
        m_fpsText.setFillColor(sf::Color::White);
        m_fpsText.setOutlineColor(sf::Color::Black);
        m_fpsText.setOutlineThickness(1.0f);
        m_fpsText.setString("FPS: --");
    }
    else
    {
        std::cerr << "! Failed to load debug font for FPS counter\n";
    }

    m_fpsClock.restart();
    m_fpsAccumTime = 0.0f;
    m_fpsFrameCount = 0;
    m_currentFps = 0.0f;
}

void State_Game::OnDestroy()
{
    EventManager& evMgr = m_stateManager.GetContext().m_eventManager;
    // TODO: OPEN MENU
    evMgr.RemoveCallback(StateType::Game, "Pause");
    evMgr.RemoveCallback(StateType::Game, "ToggleDebug");
}

void State_Game::Activate() {}

void State_Game::Deactivate() {}

void State_Game::Update(const sf::Time& time)
{
    UpdateCursor(time);
    SharedContext& context = m_stateManager.GetContext();

    EntityBase* player = nullptr;

    // Find player
    // Fast path: resolve by cached ID
    if (m_playerIdCache >= 0)
        player = context.GetEntityManager().Find(static_cast<unsigned int>(m_playerIdCache));

    // Slow path: fallback by name, then refresh cache
    if (!player)
    {
        player = context.GetEntityManager().Find("Player");
        m_playerIdCache = player ? static_cast<int>(player->GetId()) : -1;
    }

    float mapHeight = static_cast<float>(m_gameMap.GetMapSize().y * m_gameMap.GetTileSize());
    if (player)
    {
        CTransform* transform = player->GetComponent<CTransform>();
        CBoxCollider* collider = player->GetComponent<CBoxCollider>();
        CState* state = player->GetComponent<CState>();

        // Check if player components exist and player is not already dying
        if (transform && collider && state && state->GetState() != EntityState::Dying)
        {
            // Here is pBounds! We get the AABB (Axis-Aligned Bounding Box) from the collider
            sf::FloatRect pBounds = collider->GetAABB();

            // Fall death check
            if (transform->GetPosition().y >= mapHeight)
                state->InstantKill();

            // Level Transition (Touching the Door)
            if (m_gameMap.GetDoorRect().findIntersection(pBounds))
                m_gameMap.LoadNext();

            // Trap collision check
            for (const auto& trap : m_gameMap.GetTraps())
            {
                if (trap.findIntersection(pBounds))
                {
                    state->InstantKill();
                    break;
                }
            }
        }
    }
    else
    {
        std::cout << "Respawning player..." << '\n';
        int playerId = context.GetEntityManager().Add(EntityType::Player, "Player");

        if (playerId < 0)
        {
            std::cerr << "! Failed to respawn player: entity limit reached or creation failed\n";
            m_playerIdCache = -1;
        }
        else
        {
            m_playerIdCache = playerId;

            player = context.GetEntityManager().Find(static_cast<unsigned int>(playerId));
            if (player)
            {
                if (CTransform* transform = player->GetComponent<CTransform>())
                    transform->SetPosition(m_gameMap.GetPlayerStart());
            }
        }
    }


    // Update Entities
    context.GetEntityManager().Update(time.asSeconds());
    // Update the map
    m_gameMap.Update(time.asSeconds());
    // Update HUD overlay
    m_hud->Update();

    // TODO: Delegare a camera system
    //m_gameCameraView = m_stateManager.GetContext().m_window.GetRenderWindow().getView();
}

void State_Game::Draw()
{
    SharedContext& context = m_stateManager.GetContext();
    sf::RenderWindow& window = context.m_window.GetRenderWindow();
    EntityManager& entityManager = context.GetEntityManager();
    const sf::View baseGameView = context.m_window.GetGameView();
    sf::View gameView = entityManager.GetCameraSystem().GetCurrentView();

    // Keep a valid camera view even in edge cases
    if (gameView.getSize().x <= 0.0f || gameView.getSize().y <= 0.0f)
    {
        gameView = baseGameView;
    }

    // Use a single viewport source for this frame
    gameView.setViewport(baseGameView.getViewport());
    window.setView(gameView);


    m_gameMap.Draw(window);

    context.GetEntityManager().Draw();

    // --- DEBUG OVERLAY ---
    if (m_debugMode)
    {
        for (auto& entityPair : context.GetEntityManager().GetEntities())
        {
            EntityBase* entity = entityPair.second.get();
            CBoxCollider* collider = entity->GetComponent<CBoxCollider>();
            CSprite* sprite = entity->GetComponent<CSprite>();
            CState* state = entity->GetComponent<CState>();

            if (collider && sprite)
            {
                sf::FloatRect bodyRect = collider->GetAABB();

                sf::RectangleShape bodyShape(bodyRect.size);
                bodyShape.setPosition(bodyRect.position);

                bodyShape.setFillColor(sf::Color::Transparent);
                bodyShape.setOutlineColor(sf::Color::Cyan);
                bodyShape.setOutlineThickness(1.0f);
                window.draw(bodyShape);

                sf::FloatRect attackRect = collider->GetAttackAABB();
                sf::Vector2f offset = collider->GetAttackAABBOffset();

                if (attackRect.size.x > 0 && attackRect.size.y > 0)
                {
                    float attackX = 0.0f;

                    // Stessa identica matematica che usiamo nel CombatSystem!
                    if (sprite->GetDirection() == Direction::Right) {
                        attackX = (bodyRect.position.x + bodyRect.size.x) + offset.x;
                    }
                    else {
                        attackX = bodyRect.position.x - offset.x - attackRect.size.x;
                    }

                    // La Y parte dalla cima della Hitbox del corpo!
                    float attackY = bodyRect.position.y + offset.y;

                    sf::RectangleShape attackShape({ attackRect.size.x, attackRect.size.y });
                    attackShape.setPosition({ attackX, attackY });
                    attackShape.setFillColor(sf::Color::Transparent);

                    // Some entities may not have CState (e.g. projectiles)
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
        }

        // FPS Counter
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

            // Color by performance tier
            if (m_currentFps >= 55.0f)
                m_fpsText.setFillColor(sf::Color(80, 220, 120)); // Green
            else if (m_currentFps >= 30.0f)
                m_fpsText.setFillColor(sf::Color(240, 210, 80)); // Yellow
            else
                m_fpsText.setFillColor(sf::Color(230, 80, 80)); // Red
        }

        window.setView(context.m_window.GetUIView());

        const sf::Vector2f uiRes = context.m_window.GetUIResolution();
        const sf::FloatRect b = m_fpsText.getLocalBounds();
        m_fpsText.setOrigin({ b.position.x + b.size.x, b.position.y });
        m_fpsText.setPosition({ uiRes.x - 20.0f, 20.0f });

        window.draw(m_fpsText);
    }

    if (m_hud)
    {
        window.setView(m_stateManager.GetContext().m_window.GetUIView());
        m_hud->Draw(m_stateManager.GetContext().m_window.GetRenderWindow());
        // Restore gameplay view so next update/draw cycle does not start from UI view
        window.setView(gameView);
    }
}

void State_Game::MainMenu(EventDetails& details)
{
    m_stillCursorTime = 0.0f;
    SetCursorVisible(true);
    m_stateManager.SwitchTo(StateType::MainMenu);
}

void State_Game::Pause(EventDetails& details)
{
    m_stillCursorTime = 0.0f;
    SetCursorVisible(true);
        
    m_stateManager.SwitchTo(StateType::Paused);
}

void State_Game::ToggleDebugOverlay(EventDetails& details)
{
    m_debugMode = !m_debugMode;
    std::cout << "Debug Mode: " << (m_debugMode ? "ON" : "OFF") << "\n";

    if (m_debugMode)
    {
        m_fpsClock.restart();
        m_fpsAccumTime = 0.0f;
        m_fpsFrameCount = 0;
        m_currentFps = 0.0f;
        if (m_debugFontLoaded) m_fpsText.setString("FPS: --");
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