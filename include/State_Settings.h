#pragma once
#include "BaseState.h"
#include "EventManager.h"
#include <SFML/Graphics.hpp>
#include <optional>
#include <vector>
#include <string>

struct SettingsButton {
    SettingsButton(const sf::Font& font) : label(font) {}
    sf::RectangleShape rect;
    sf::Text label;
};

class State_Settings : public BaseState
{
public:
    State_Settings(StateManager& stateManager);
    ~State_Settings() override;

    void OnCreate() override;
    void OnDestroy() override;
    void Activate() override;
    void Deactivate() override;
    void Update(const sf::Time& time) override;
    void Draw() override;

    void MouseClick(EventDetails& details);

private:
    void UpdateVolumeText();

    sf::Font m_fontTitle;
    sf::Font m_fontButton;

    sf::Text m_title;
    sf::Text m_volumeLabel;

    std::optional<sf::Sprite> m_backgroundSprite;
    sf::RectangleShape m_overlay;

    std::vector<SettingsButton> m_buttons;

    int m_volume;

    const int MAX_VOLUME = 100;
    const int MIN_VOLUME = 0;
    const int VOLUME_STEP = 10;

    // Layout
    const float TITLE_Y_RATIO = 0.15f;
    const float CONTROLS_Y_RATIO = 0.40f;
    const float BACK_BTN_Y_RATIO = 0.80f;
};