#pragma once
#include <optional>
#include "BaseState.h"
#include <SFML/Graphics.hpp>

struct EventDetails;

class State_Credits : public BaseState
{
public:
    State_Credits(StateManager& stateManager);
    ~State_Credits() override;

    void OnCreate() override;
    void OnDestroy() override;
    void Activate() override;
    void Deactivate() override;
    void Update(const sf::Time& time) override;
    void Draw() override;

    void ReturnToMenu(EventDetails& details);

private:
    sf::Font m_fontTitle;
    sf::Font m_fontBody;

    sf::Text m_title;
    sf::Text m_creditsText;
    sf::Text m_promptText;

    std::optional<sf::Sprite> m_backgroundSprite;
    sf::RectangleShape m_overlay;

    float m_scrollPos;
    float m_timePassed;

    const float SCROLL_SPEED = 70.0f;
    const float BLINK_SPEED = 3.0f;

    const std::string CREDITS_CONTENT =
        "LEAD DEVELOPER\nMonkeyDigit\n\n"
        "ENGINE\nNocturne Engine (C++ / SFML 3)\n\n"
        "ARCHITECTURE\nEntity-Component-System\n\n"
        "SPECIAL THANKS\nTo the sleepless nights\nand lots of coffee!";
};