#pragma once
#include <SFML/Graphics.hpp>
#include <optional>
#include "EventManager.h"
#include "BaseState.h"

class State_Intro : public BaseState
{
public:
    State_Intro(StateManager& stateManager);
    ~State_Intro() override;

    void OnCreate() override;
    void OnDestroy() override;
    void Activate() override;
    void Deactivate() override;
    void Update(const sf::Time& time) override;
    void Draw() override;

    void Continue(EventDetails&);

private:
    sf::Font m_font;
    sf::Text m_text;
    std::optional<sf::Sprite> m_introSprite;
    std::optional<sf::Sprite> m_backgroundSprite;

    float m_timePassed;
    float m_alpha;

    const float ANIMATION_DURATION = 2.0f;
    const float PULSE_SPEED = 90.0f;
};