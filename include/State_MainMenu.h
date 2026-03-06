#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <optional>
#include "BaseState.h"
#include "EventManager.h"

struct MenuButton {
    MenuButton(const sf::Font& font) : label(font) {}
    sf::RectangleShape rect;
    sf::Text label;
};

class State_MainMenu : public BaseState
{
public:
    State_MainMenu(StateManager& stateManager);
    ~State_MainMenu() override;

    void OnCreate() override;
    void OnDestroy() override;
    void Activate() override;
    void Deactivate() override;
    void Update(const sf::Time& time) override;
    void Draw() override;

    void MouseClick(EventDetails& details);

private:
    sf::Font m_fontTitle;
    sf::Font m_fontButton;
    sf::Text m_title;
    std::optional<sf::Sprite> m_backgroundSprite;

    sf::Vector2f m_buttonSize;
    sf::Vector2f m_buttonPos;
    unsigned int m_buttonPadding;

    std::vector<MenuButton> m_buttons;
};