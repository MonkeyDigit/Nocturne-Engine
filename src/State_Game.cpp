#include <iostream>
#include <cmath>
#include "State_Game.h"
#include "StateManager.h"
#include "SharedContext.h"
#include "Window.h"
#include "EntityManager.h"
#include "CState.h"
#include <SFML/Graphics/RectangleShape.hpp>

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

    m_gameMap.LoadMap("media/maps/map1.map");

    // TODO: AGGIUNGERE L'HEALTH BAR
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

    // Camera Tracking & Respawn
    EntityBase* player = context.m_entityManager.Find("Player");

    float mapHeight = static_cast<float>(m_gameMap.GetMapSize().y * m_gameMap.GetTileSize());
    if (player)
    {
        CTransform* transform = player->GetComponent<CTransform>();
        if (transform)
        {
            sf::Vector2f playerPos = transform->GetPosition();
            sf::Vector2f playerSize = transform->GetSize();

            // --- OUT OF BOUNDS CHECK ---
            if (playerPos.y >= mapHeight)
            {
                CState* state = player->GetComponent<CState>();
                if (state && state->GetState() != EntityState::Dying)
                    state->InstantKill();
            }
            else
                m_view.setCenter({ playerPos.x, playerPos.y + playerSize.y * 0.5f });
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

    // --- CLAMPING CAMERA ---
    sf::Vector2f viewCenter = m_view.getCenter();
    sf::Vector2f viewSize = m_view.getSize();

    float mapWidth = static_cast<float>(m_gameMap.GetMapSize().x * m_gameMap.GetTileSize());

    // Horizontal bounds
    if (viewCenter.x - viewSize.x * 0.5f < 0.0f)
        viewCenter.x = viewSize.x * 0.5f;
    else if (viewCenter.x + viewSize.x * 0.5f > mapWidth)
        viewCenter.x = mapWidth - viewSize.x * 0.5f;

    // Vertical bounds
    if (viewCenter.y + viewSize.y * 0.5f > mapHeight)
        viewCenter.y = mapHeight - viewSize.y * 0.5f;
    else if (viewCenter.y - viewSize.y * 0.5f < 0.0f)
        viewCenter.y = viewSize.y * 0.5f;

    m_view.setCenter(viewCenter);

    // Apply view
    context.m_window.GetRenderWindow().setView(m_view);

    // Update the map
    m_gameMap.Update(time.asSeconds());
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

    // --- UI OVERLAY ---

    sf::View currentView = window.getView();

    window.setView(window.getDefaultView());

    EntityBase* player = context.m_entityManager.Find("Player");
    if (player)
    {
        CState* state = player->GetComponent<CState>();
        if (state)
        {
            int hp = state->GetHitPoints();
            int maxHp = state->GetMaxHitPoints();

            float hpPercent = static_cast<float>(hp) / static_cast<float>(maxHp);
            if (hpPercent < 0.0f) hpPercent = 0.0f;

            sf::RectangleShape bgBar({ 200.0f, 20.0f });
            bgBar.setPosition({ 20.0f, 20.0f });
            bgBar.setFillColor(sf::Color(50, 50, 50, 200));

            sf::RectangleShape fgBar({ 200.0f * hpPercent, 20.0f });
            fgBar.setPosition({ 20.0f, 20.0f });
            fgBar.setFillColor(sf::Color(220, 50, 50, 255));

            window.draw(bgBar);
            window.draw(fgBar);
        }
    }
    window.setView(currentView);
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