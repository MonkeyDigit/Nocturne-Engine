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
#include "CTransform.h"
#include "CBoxCollider.h"
#include "CombatGeometry.h"

namespace
{
    constexpr unsigned int kFpsTextCharacterSize = 20u;
    constexpr float kFpsTextOutlineThickness = 1.0f;
    constexpr float kFpsSampleIntervalSeconds = 0.25f;
    constexpr float kFpsGoodThreshold = 55.0f;
    constexpr float kFpsWarnThreshold = 30.0f;
    constexpr float kFpsMargin = 20.0f;

    constexpr float kBodyOutlineThickness = 1.0f;
    constexpr float kAttackActiveOutlineThickness = 2.0f;
    constexpr float kAttackInactiveOutlineThickness = 1.0f;

    const sf::Color kFpsGoodColor(80, 220, 120);
    const sf::Color kFpsWarnColor(240, 210, 80);
    const sf::Color kFpsBadColor(230, 80, 80);
}

State_Game::State_Game(StateManager& stateManager)
    : BaseState(stateManager),
    m_stillCursorTime(0.0f),
    m_cursorVisible(true),
    m_debugMode(false),
    m_gameMap(stateManager.GetContext(), this),
    m_fpsText(m_debugFont),
    m_fpsAccumTime(0.0f),
    m_fpsFrameCount(0),
    m_currentFps(0.0f),
    m_debugFontLoaded(false),
    m_runElapsedSeconds(0.0f),
    m_runKills(0u),
    m_runDamageTaken(0u)
{
}

State_Game::~State_Game() {}

void State_Game::Update(const sf::Time& time)
{
    UpdateCursor(time);
    UpdateRunStats(time.asSeconds());

    if (!m_gameMap.IsNextMapQueued())
    {
        EntityBase* player = ResolvePlayer();

        if (player)
        {
            HandlePlayerHazards(*player);
        }
        else
        {
            // Count one death when the player no longer exists in the world
            ++m_stateManager.GetContext().m_sessionStats.m_deaths;

            m_stillCursorTime = 0.0f;
            SetCursorVisible(true);
            m_stateManager.SwitchTo(StateType::GameOver);
            m_stateManager.Remove(StateType::Game);
            return;
        }
    }

    SharedContext& context = m_stateManager.GetContext();
    context.GetEntityManager().Update(time.asSeconds());
    m_gameMap.Update();

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

void State_Game::InitializeDebugOverlay()
{
    m_debugFontLoaded = LoadFontOrWarn(
        m_debugFont,
        "media/fonts/EightBitDragon.ttf",
        "State_Game",
        "debug_fps");

    m_fpsText.setCharacterSize(kFpsTextCharacterSize);
    m_fpsText.setFillColor(sf::Color::White);
    m_fpsText.setOutlineColor(sf::Color::Black);
    m_fpsText.setOutlineThickness(kFpsTextOutlineThickness);
    m_fpsText.setString("FPS: --\nFrame: -- ms\nUpd/frame: --\nFixed: --\nDrops: 0\nRun: 0 s\nKills: 0\nDeaths: 0\nDmgTaken: 0");

    ResetFpsCounter();
}

void State_Game::ResetFpsCounter()
{
    m_fpsClock.restart();
    m_fpsAccumTime = 0.0f;
    m_fpsFrameCount = 0;
    m_currentFps = 0.0f;

    if (m_debugFontLoaded)
        m_fpsText.setString("FPS: --\nFrame: -- ms\nUpd/frame: --\nFixed: --\nDrops: 0\nRun: 0 s\nKills: 0\nDeaths: 0\nDmgTaken: 0");
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
        bodyShape.setOutlineThickness(kBodyOutlineThickness);

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
            attackShape.setOutlineThickness(kAttackActiveOutlineThickness);
        }
        else
        {
            attackShape.setOutlineColor(sf::Color::Yellow);
            attackShape.setOutlineThickness(kAttackInactiveOutlineThickness);
        }

        window.draw(attackShape);
    }
}

void State_Game::DrawFpsCounter(sf::RenderWindow& window, const sf::View& gameView)
{
    if (!m_debugFontLoaded) return;

    SharedContext& context = m_stateManager.GetContext();
    const RuntimeFrameStats& loopStats = context.m_runtimeFrameStats;

    const float dt = m_fpsClock.restart().asSeconds();
    m_fpsAccumTime += dt;
    ++m_fpsFrameCount;

    if (m_fpsAccumTime >= kFpsSampleIntervalSeconds)
    {
        m_currentFps = static_cast<float>(m_fpsFrameCount) / m_fpsAccumTime;
        m_fpsAccumTime = 0.0f;
        m_fpsFrameCount = 0;

        const float fixedHz = (loopStats.m_fixedStepSeconds > 0.0f)
            ? (1.0f / loopStats.m_fixedStepSeconds)
            : 0.0f;

        const float frameMs = std::max(0.0f, loopStats.m_elapsedFrameSeconds * 1000.0f);

        std::ostringstream ss;
        ss << "FPS: " << static_cast<int>(std::round(m_currentFps))
            << "\nFrame: " << static_cast<int>(std::round(frameMs)) << " ms"
            << "\nUpd/frame: " << loopStats.m_lastFixedUpdates << "/" << loopStats.m_maxUpdatesPerFrame
            << "\nFixed: " << static_cast<int>(std::round(fixedHz)) << " Hz"
            << "\nDrops: " << loopStats.m_backlogDropCount
            << "\nRun: " << static_cast<int>(std::round(m_runElapsedSeconds)) << " s"
            << "\nKills: " << m_runKills
            << "\nDeaths: " << context.m_sessionStats.m_deaths
            << "\nDmgTaken: " << m_runDamageTaken;
        m_fpsText.setString(ss.str());

        if (loopStats.m_droppedBacklog || m_currentFps < kFpsWarnThreshold)
            m_fpsText.setFillColor(kFpsBadColor);
        else if (m_currentFps < kFpsGoodThreshold)
            m_fpsText.setFillColor(kFpsWarnColor);
        else
            m_fpsText.setFillColor(kFpsGoodColor);
    }

    m_fpsText.setOutlineColor(loopStats.m_droppedBacklog ? kFpsBadColor : sf::Color::Black);

    window.setView(context.m_window.GetUIView());

    const sf::Vector2f uiRes = context.m_window.GetUIResolution();
    const sf::FloatRect b = m_fpsText.getLocalBounds();
    m_fpsText.setOrigin({ b.position.x + b.size.x, b.position.y });
    m_fpsText.setPosition({ uiRes.x - kFpsMargin, kFpsMargin });

    window.draw(m_fpsText);
    window.setView(gameView);
}

sf::View State_Game::ResolveGameView()
{
    SharedContext& context = m_stateManager.GetContext();
    EntityManager& entityManager = context.GetEntityManager();

    const sf::View baseGameView = context.m_window.GetGameView();
    sf::View gameView = entityManager.GetCameraSystem().GetCurrentView();

    if (gameView.getSize().x <= 0.0f || gameView.getSize().y <= 0.0f)
        gameView = baseGameView;

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

    window.setView(gameView);
}

void State_Game::ResetRunStats()
{
    m_runElapsedSeconds = 0.0f;
    m_runKills = 0u;
    m_runDamageTaken = 0u;
    m_prevHitPoints.clear();
    m_countedDyingEntities.clear();
}

void State_Game::UpdateRunStats(float deltaSeconds)
{
    if (!std::isfinite(deltaSeconds) || deltaSeconds <= 0.0f)
        return;

    m_runElapsedSeconds += deltaSeconds;

    SharedContext& context = m_stateManager.GetContext();
    EntityManager& entityManager = context.GetEntityManager();

    std::unordered_set<unsigned int> aliveIds;
    aliveIds.reserve(entityManager.GetEntities().size());

    for (auto& [id, entityPtr] : entityManager.GetEntities())
    {
        if (!entityPtr) continue;

        EntityBase* entity = entityPtr.get();
        CState* state = entity->GetComponent<CState>();
        if (!state) continue;

        aliveIds.insert(id);

        const int currentHp = state->GetHitPoints();
        auto prevHpIt = m_prevHitPoints.find(id);

        if (prevHpIt != m_prevHitPoints.end())
        {
            const int prevHp = prevHpIt->second;

            // Track only damage taken by player
            if (entity->GetType() == EntityType::Player && currentHp < prevHp)
                m_runDamageTaken += static_cast<unsigned int>(prevHp - currentHp);
        }

        m_prevHitPoints[id] = currentHp;

        // Count each entity entering Dying only once
        if (state->GetState() == EntityState::Dying &&
            m_countedDyingEntities.find(id) == m_countedDyingEntities.end())
        {
            if (entity->GetType() == EntityType::Enemy)
            {
                // Enemy suicides/environmental deaths can force Dying without HP reaching 0
                // Count only deaths that actually depleted HP
                if (currentHp <= 0)
                    ++m_runKills;
            }

            m_countedDyingEntities.insert(id);
        }
    }

    for (auto it = m_prevHitPoints.begin(); it != m_prevHitPoints.end();)
    {
        if (aliveIds.find(it->first) == aliveIds.end())
            it = m_prevHitPoints.erase(it);
        else
            ++it;
    }

    for (auto it = m_countedDyingEntities.begin(); it != m_countedDyingEntities.end();)
    {
        if (aliveIds.find(*it) == aliveIds.end())
            it = m_countedDyingEntities.erase(it);
        else
            ++it;
    }
}