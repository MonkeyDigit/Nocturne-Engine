#pragma once
#include <SFML/Graphics.hpp>
#include "BaseState.h"

struct EventDetails;

class State_Paused : public BaseState
{
public:
    State_Paused(StateManager& stateManager);
    ~State_Paused() override;

    void OnCreate() override;
    void OnDestroy() override;
    void Activate() override;
    void Deactivate() override;
    void Update(const sf::Time&) override;
    void Draw() override;

    void Unpause(EventDetails&);
    void QuitToMainMenu(EventDetails&);

private:
    sf::Font m_font;
    sf::Text m_text;
    sf::RectangleShape m_rect;
};