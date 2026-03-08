#pragma once
#include <memory>
#include "BaseState.h"
#include "EventManager.h"
#include "Map.h"
#include "HUD.h"

class EntityBase;

class State_Game : public BaseState
{
public:
    State_Game(StateManager& stateManager);
    ~State_Game() override;

    void OnCreate() override;
    void OnDestroy() override;
    void Activate() override;
    void Deactivate() override;
    void Update(const sf::Time& time) override;
    void Draw() override;

    // Callbacks for EventManager
    void MainMenu(EventDetails&);
    void Pause(EventDetails&);
    void ToggleDebugOverlay(EventDetails&);

private:
    void UpdateCursor(const sf::Time& time);
    void SetCursorVisible(bool visible);

    void InitializeDebugOverlay();
    void ResetFpsCounter();
    void DrawDebugOverlay(sf::RenderWindow& window, const sf::View& gameView);
    void DrawDebugHitboxes(sf::RenderWindow& window);
    void DrawFpsCounter(sf::RenderWindow& window, const sf::View& gameView);

    EntityBase* ResolvePlayer();
    EntityBase* RespawnPlayer();
    void HandlePlayerHazards(EntityBase& player);

    sf::View ResolveGameView();
    void ApplyGameView(sf::RenderWindow& window, const sf::View& gameView);
    void DrawHudOverlay(sf::RenderWindow& window, const sf::View& gameView);

    float m_stillCursorTime;
    sf::Vector2i m_mousePos;
    bool m_cursorVisible;

    Map m_gameMap;
    std::unique_ptr<HUD> m_hud;
    bool m_debugMode;

    int m_playerIdCache;

    sf::Font m_debugFont;
    sf::Text m_fpsText;
    sf::Clock m_fpsClock;
    float m_fpsAccumTime;
    unsigned int m_fpsFrameCount;
    float m_currentFps;
    bool m_debugFontLoaded;

};