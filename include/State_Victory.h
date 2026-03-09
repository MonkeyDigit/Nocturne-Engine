#pragma once
#include <optional>
#include <SFML/Graphics.hpp>
#include "BaseState.h"

struct EventDetails;

class State_Victory : public BaseState
{
public:
    State_Victory(StateManager& stateManager);
    ~State_Victory() override;

    void OnCreate() override;
    void OnDestroy() override;
    void Activate() override;
    void Deactivate() override;
    void Update(const sf::Time& time) override;
    void Draw() override;

    void GoToMainMenu(EventDetails& details);
    void GoToCredits(EventDetails& details);

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