#include "State_Settings.h"
#include "StateManager.h"
#include "SharedContext.h"
#include "Window.h"
#include "TextureManager.h"
#include "AudioManager.h"
#include <iostream>

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
    m_volume = context.m_audioManager.GetMasterVolume();
    UpdateVolumeText();

    // Background and overlay
    context.m_textureManager.RequireResource("MenuBg");
    sf::Texture* bgTex = context.m_textureManager.GetResource("MenuBg");
    if (bgTex) {
        m_backgroundSprite.emplace(*bgTex);
        float scale = uiRes.y / bgTex->getSize().y;
        m_backgroundSprite->setScale({ scale, scale });
        m_backgroundSprite->setOrigin({ m_backgroundSprite->getLocalBounds().size.x * 0.5f, m_backgroundSprite->getLocalBounds().size.y * 0.5f });
        m_backgroundSprite->setPosition(uiRes * 0.5f);
    }

    m_overlay.setSize(uiRes);
    m_overlay.setFillColor(sf::Color(0, 0, 0, 150));

    // Load fonts
    if (!m_fontTitle.openFromFile("media/fonts/OLDENGL.ttf")) {
        std::cerr << "! Failed to load title font\n";
    }
    if (!m_fontButton.openFromFile("media/fonts/EightBitDragon.ttf")) {
        std::cerr << "! Failed to load button font\n";
    }

    // Title
    m_title.setString("SETTINGS");
    m_title.setCharacterSize(80);
    sf::FloatRect titleRect = m_title.getLocalBounds();
    m_title.setOrigin({ titleRect.position.x + titleRect.size.x * 0.5f, titleRect.position.y + titleRect.size.y * 0.5f });
    m_title.setPosition({ uiRes.x * 0.5f, uiRes.y * TITLE_Y_RATIO });

    // Volume label on center
    m_volumeLabel.setCharacterSize(40);
    UpdateVolumeText();
    m_volumeLabel.setPosition({ uiRes.x * 0.5f, uiRes.y * CONTROLS_Y_RATIO });

    // Buttons
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
        btn.rect.setSize(btnSizes[i]);
        btn.rect.setFillColor(sf::Color(0, 0, 128, 160));
        btn.rect.setOutlineColor(sf::Color(0, 0, 64));
        btn.rect.setOutlineThickness(2.0f);
        btn.rect.setOrigin({ btnSizes[i].x * 0.5f, btnSizes[i].y * 0.5f });
        btn.rect.setPosition(btnPos[i]);

        btn.label.setString(btnNames[i]);
        btn.label.setCharacterSize(30);
        sf::FloatRect labelRect = btn.label.getLocalBounds();
        btn.label.setOrigin({ labelRect.position.x + labelRect.size.x * 0.5f, labelRect.position.y + labelRect.size.y * 0.5f });
        btn.label.setPosition(btn.rect.getPosition());

        m_buttons.push_back(std::move(btn));
    }

    context.m_eventManager.AddCallback(StateType::Settings, "Left_Click", &State_Settings::MouseClick, *this);
}

void State_Settings::OnDestroy()
{
    m_stateManager.GetContext().m_eventManager.RemoveCallback(StateType::Settings, "Left_Click");
    m_stateManager.GetContext().m_textureManager.ReleaseResource("MenuBg");
}

void State_Settings::Activate() {}
void State_Settings::Deactivate() {}

void State_Settings::Update(const sf::Time& time)
{
    sf::RenderWindow& window = m_stateManager.GetContext().m_window.GetRenderWindow();
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    sf::View uiView = m_stateManager.GetContext().m_window.GetUIView();
    sf::Vector2f mouseWorldPos = window.mapPixelToCoords(mousePos, uiView);

    for (auto& btn : m_buttons) {
        if (btn.rect.getGlobalBounds().contains(mouseWorldPos))
            btn.rect.setFillColor(sf::Color(128, 0, 0, 160));
        else
            btn.rect.setFillColor(sf::Color(0, 0, 128, 160));
    }
}

void State_Settings::Draw()
{
    sf::RenderWindow& window = m_stateManager.GetContext().m_window.GetRenderWindow();

    if (m_backgroundSprite) window.draw(*m_backgroundSprite);
    window.draw(m_overlay);
    window.draw(m_title);
    window.draw(m_volumeLabel);

    for (const auto& btn : m_buttons) {
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
    sf::FloatRect rect = m_volumeLabel.getLocalBounds();
    m_volumeLabel.setOrigin({ rect.position.x + rect.size.x * 0.5f, rect.position.y + rect.size.y * 0.5f });
}