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
    m_gameMap(stateManager.GetContext(), this)
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

    m_hud = std::make_unique<HUD>(m_stateManager.GetContext().m_entityManager);
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

    EntityBase* player = context.m_entityManager.Find("Player");
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
        int playerId = context.m_entityManager.Add(EntityType::Player, "Player");
        player = context.m_entityManager.Find(playerId);

        if (player)
        {
            CTransform* transform = player->GetComponent<CTransform>();
            if (transform) transform->SetPosition(m_gameMap.GetPlayerStart());
        }
    }

    // Update Entities
    context.m_entityManager.Update(time.asSeconds());
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
    window.setView(m_stateManager.GetContext().m_entityManager.GetCameraView());

    m_gameMap.Draw(window);

    context.m_entityManager.Draw();

    // --- DEBUG OVERLAY ---
    if (m_debugMode)
    {
        for (auto& entityPair : context.m_entityManager.GetEntities())
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

                    if (state->GetState() == EntityState::Attacking) {
                        attackShape.setOutlineColor(sf::Color::Red);
                        attackShape.setOutlineThickness(2.0f);
                    }
                    else {
                        attackShape.setOutlineColor(sf::Color::Yellow);
                        attackShape.setOutlineThickness(1.0f);
                    }

                    window.draw(attackShape);
                }
            }
        }
    }

    if (m_hud)
    {
        window.setView(m_stateManager.GetContext().m_window.GetUIView());
        m_hud->Draw(m_stateManager.GetContext().m_window.GetRenderWindow());
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