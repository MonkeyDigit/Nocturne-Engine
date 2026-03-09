#include <algorithm>
#include <cmath>
#include "State_Settings.h"
#include "StateManager.h"
#include "SharedContext.h"
#include "Window.h"
#include "AudioManager.h"

State_Settings::State_Settings(StateManager& stateManager)
    : BaseState(stateManager),
    m_title(m_fontTitle),
    m_volumeLabel(m_fontButton),
    m_volume(100)
{
}

State_Settings::~State_Settings() {}

void State_Settings::OnCreate()
{
    SharedContext& context = m_stateManager.GetContext();
    sf::Vector2f uiRes = context.m_window.GetUIResolution();
    const float masterVolume = context.m_audioManager.GetMasterVolume();
    m_volume = std::clamp(static_cast<int>(std::lround(masterVolume)), MIN_VOLUME, MAX_VOLUME);

    // Background and overlay
    SetupCenteredBackground(
        m_backgroundSprite,
        "MenuBg",
        uiRes,
        "State_Settings",
        true);

    m_overlay.setSize(uiRes);
    m_overlay.setFillColor(sf::Color(0, 0, 0, 150));

    // Load fonts
    LoadFontOrWarn(m_fontTitle, "media/fonts/OLDENGL.ttf", "State_Settings", "title");
    LoadFontOrWarn(m_fontButton, "media/fonts/EightBitDragon.ttf", "State_Settings", "buttons");

    // Title
    m_title.setString("SETTINGS");
    m_title.setCharacterSize(80);
    CenterText(m_title, uiRes.x * 0.5f, uiRes.y * TITLE_Y_RATIO);

    // Volume label on center
    m_volumeLabel.setCharacterSize(40);
    m_volumeLabel.setPosition({ uiRes.x * 0.5f, uiRes.y * CONTROLS_Y_RATIO });

    // SETUP BUTTONS
    std::vector<std::string> btnNames = { "-", "+", "BACK TO MENU" };
    sf::Vector2f btnSizes[] = { {60.f, 60.f}, {60.f, 60.f}, {300.f, 60.f} };

    // Dynamic layout
    float centerX = uiRes.x * 0.5f;
    float controlsY = uiRes.y * CONTROLS_Y_RATIO;
    float backBtnY = uiRes.y * BACK_BTN_Y_RATIO;

    sf::Vector2f btnPos[] = {
        { centerX - 300.f, controlsY }, // Minus button
        { centerX + 300.f, controlsY }, // Plus button
        { centerX, backBtnY }           // Back button
    };

    for (size_t i = 0; i < 3; ++i) {
        SettingsButton btn(m_fontButton);

        SetupTextButton(
            btn.rect,
            btn.label,
            btnSizes[i],
            btnPos[i],
            btnNames[i],
            30u);

        m_buttons.push_back(std::move(btn));
    }

    context.m_eventManager.AddCallback(StateType::Settings, "Left_Click", &State_Settings::MouseClick, *this);
}

void State_Settings::OnDestroy()
{
    m_stateManager.GetContext().m_eventManager.RemoveCallback(StateType::Settings, "Left_Click");
    ReleaseTrackedTextures("State_Settings");
}

void State_Settings::Activate() {}
void State_Settings::Deactivate() {}

void State_Settings::Update(const sf::Time&)
{
    sf::RenderWindow& window = m_stateManager.GetContext().m_window.GetRenderWindow();
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    sf::View uiView = m_stateManager.GetContext().m_window.GetUIView();
    sf::Vector2f mouseWorldPos = window.mapPixelToCoords(mousePos, uiView);

    for (auto& btn : m_buttons)
        UpdateButtonHoverColor(btn.rect, mouseWorldPos);
}

void State_Settings::Draw()
{
    sf::RenderWindow& window = m_stateManager.GetContext().m_window.GetRenderWindow();
    window.setView(m_stateManager.GetContext().m_window.GetUIView());

    if (m_backgroundSprite) window.draw(*m_backgroundSprite);
    window.draw(m_overlay);
    window.draw(m_title);
    window.draw(m_volumeLabel);

    for (const auto& btn : m_buttons)
    {
        window.draw(btn.rect);
        window.draw(btn.label);
    }
}


void State_Settings::MouseClick(EventDetails& details)
{
    sf::RenderWindow& window = m_stateManager.GetContext().m_window.GetRenderWindow();
    sf::View uiView = m_stateManager.GetContext().m_window.GetUIView();
    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Vector2i(details.m_mouse.x, details.m_mouse.y), uiView);

    for (size_t i = 0; i < m_buttons.size(); ++i)
    {
        if (m_buttons[i].rect.getGlobalBounds().contains(mousePos))
        {
            if (i == 0) { // Minus button
                m_volume = std::max(MIN_VOLUME, m_volume - VOLUME_STEP);
                UpdateVolumeText();
                m_stateManager.GetContext().m_audioManager.SetMasterVolume(static_cast<float>(m_volume));
            }
            else if (i == 1) { // Plus button
                m_volume = std::min(MAX_VOLUME, m_volume + VOLUME_STEP);
                UpdateVolumeText();
                m_stateManager.GetContext().m_audioManager.SetMasterVolume(static_cast<float>(m_volume));
            }
            else if (i == 2) { // Back button
                m_stateManager.SwitchTo(StateType::MainMenu);
            }
            break;
        }
    }
}

void State_Settings::UpdateVolumeText()
{
    m_volumeLabel.setString("VOLUME: " + std::to_string(m_volume) + "%");
    const sf::Vector2f pos = m_volumeLabel.getPosition();
    CenterText(m_volumeLabel, pos.x, pos.y);
}