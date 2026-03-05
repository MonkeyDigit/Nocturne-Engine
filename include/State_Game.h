#pragma once
#include <memory>
#include "BaseState.h"
#include "EventManager.h"
#include "Map.h"
#include "HUD.h"

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
    void MainMenu(EventDetails& details);
    void Pause(EventDetails& details);
    void ToggleOverlay(EventDetails& details);

private:
    void UpdateCursor(const sf::Time& time);
    void SetCursorVisible(bool visible);

    float m_stillCursorTime;
    sf::Vector2i m_mousePos;
    bool m_cursorVisible;

    Map m_gameMap;
    std::unique_ptr<HUD> m_hud;
    bool m_debugMode;
};