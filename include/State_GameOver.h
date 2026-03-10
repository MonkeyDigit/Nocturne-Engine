#pragma once
#include <optional>
#include <SFML/Graphics.hpp>
#include "BaseState.h"

struct EventDetails;

class State_GameOver : public BaseState
{
public:
    State_GameOver(StateManager& stateManager);
    ~State_GameOver() override;

    void OnCreate() override;
    void OnDestroy() override;
    void Activate() override;
    void Deactivate() override;
    void Update(const sf::Time& time) override;
    void Draw() override;

    void Retry(EventDetails&);
    void GoToMainMenu(EventDetails&);

private:
    sf::Font m_fontTitle;
    sf::Font m_fontBody;

    sf::Text m_title;
    sf::Text m_subtitle;
    sf::Text m_prompt;

    std::optional<sf::Sprite> m_backgroundSprite;
    sf::RectangleShape m_overlay;

    float m_timePassed;
};