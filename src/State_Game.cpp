#include <iostream>
#include <cmath>
#include <SFML/Graphics/RectangleShape.hpp>
#include "State_Game.h"
#include "StateManager.h"
#include "SharedContext.h"
#include "Window.h"
#include "EntityManager.h"
#include "CState.h"

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
    m_view.setSize({ 480.0f, 270.0f });
    m_view.setCenter(m_view.getSize() * 0.5f);
    AdjustView();

    EventManager& evMgr = m_stateManager.GetContext().m_eventManager;

    // Registering callbacks using the modern EventDetails reference
    // TODO: Make sure "Pause" and "ToggleDebug" are in your Bindings.cfg
    // TODO: OPEN MENU
    evMgr.AddCallback(StateType::Game, "Pause", &State_Game::Pause, *this);
    evMgr.AddCallback(StateType::Game, "ToggleDebug", &State_Game::ToggleOverlay, *this);

    // Mouse logic
    m_stillCursorTime = 0.0f;
    m_mousePos = evMgr.GetMousePos();
    m_cursorVisible = true;

    m_stateManager.GetContext().m_window.GetRenderWindow().setView(m_view);

    m_gameMap.LoadMap("media/maps/new_map.tmj");

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
        if (transform && transform->GetPosition().y >= mapHeight)
        {
            CState* state = player->GetComponent<CState>();
            if (state && state->GetState() != EntityState::Dying)
                state->InstantKill();
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
}

void State_Game::Draw()
{
    SharedContext& context = m_stateManager.GetContext();
    sf::RenderWindow& window = context.m_window.GetRenderWindow();

    m_gameMap.Draw(window);

    context.m_entityManager.Draw();

    // --- DEBUG OVERLAY ---
    if (m_debugMode)
    {
        for (auto& entityPair : context.m_entityManager.GetEntities())
        {
            EntityBase* entity = entityPair.second.get();
            CBoxCollider* collider = entity->GetComponent<CBoxCollider>();

            if (collider)
            {
                sf::FloatRect aabb = collider->GetAABB();

                sf::RectangleShape rect(aabb.size);
                rect.setPosition(aabb.position);

                rect.setFillColor(sf::Color::Transparent);
                rect.setOutlineColor(sf::Color::Cyan);
                rect.setOutlineThickness(1.0f);
                window.draw(rect);

                sf::FloatRect attackAABB = collider->GetAttackAABB();

                if (attackAABB.size.x > 0 && attackAABB.size.y > 0)
                {
                    sf::RectangleShape attRect(attackAABB.size);
                    attRect.setPosition(attackAABB.position);
                    attRect.setFillColor(sf::Color(255, 0, 0, 80));
                    attRect.setOutlineColor(sf::Color::Red);
                    attRect.setOutlineThickness(1.0f);
                    window.draw(attRect);
                }
            }
        }
    }

    if (m_hud) 
        m_hud->Draw(m_stateManager.GetContext().m_window.GetRenderWindow());
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

void State_Game::ToggleOverlay(EventDetails& details)
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