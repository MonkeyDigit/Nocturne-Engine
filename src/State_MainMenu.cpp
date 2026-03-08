#include "State_MainMenu.h"
#include "StateManager.h"
#include "SharedContext.h"
#include "Window.h"
#include "TextureManager.h"
#include <iostream>
#include "EngineLog.h"

State_MainMenu::State_MainMenu(StateManager& stateManager)
    : BaseState(stateManager), m_title(m_fontTitle), m_buttonPadding(20)
{}

State_MainMenu::~State_MainMenu() {}

void State_MainMenu::OnCreate()
{
    SharedContext& context = m_stateManager.GetContext();

    sf::Vector2f uiRes = context.m_window.GetUIResolution();

    // Load Background
    context.m_textureManager.RequireResource("MenuBg");
    sf::Texture* texture = context.m_textureManager.GetResource("MenuBg");
    if (texture)
    {
        m_backgroundSprite.emplace(*texture);
        m_backgroundSprite->setOrigin({ m_backgroundSprite->getLocalBounds().size.x * 0.5f, m_backgroundSprite->getLocalBounds().size.y * 0.5f });
        m_backgroundSprite->setPosition(uiRes * 0.5f);
    }

    // Load fonts
    if (!m_fontTitle.openFromFile("media/fonts/EightBitDragon.ttf"))
        EngineLog::WarnOnce("font.credits.title_failed", "Failed to load title font");

    if (!m_fontButton.openFromFile("media/fonts/EightBitDragon.ttf"))
        EngineLog::WarnOnce("font.mainmenu.button_failed", "Failed to load button font");

    m_title.setString("Nocturne Engine");
    m_title.setCharacterSize(80);
    m_title.setStyle(sf::Text::Bold);
    m_title.setOutlineThickness(2);
    m_title.setFillColor(sf::Color::Yellow);
    m_title.setOutlineColor(sf::Color::Red);
    sf::FloatRect textRect = m_title.getLocalBounds();
    m_title.setOrigin({ textRect.position.x + textRect.size.x * 0.5f,
                       textRect.position.y + textRect.size.y * 0.5f });
    m_title.setPosition({ uiRes.x * 0.5f, uiRes.y * 0.2f });

    // Buttons
    m_buttonSize = sf::Vector2f(300.0f, 60.0f);
    m_buttonPos = sf::Vector2f(uiRes.x * 0.5f, uiRes.y * 0.4f);

    std::vector<std::string> buttonNames = { "PLAY", "CREDITS", "SETTINGS", "EXIT" };

    for (size_t i = 0; i < buttonNames.size(); ++i)
    {
        MenuButton btn(m_fontButton);

        btn.rect.setSize(m_buttonSize);
        btn.rect.setFillColor(sf::Color(0, 0, 128, 160));
        btn.rect.setOutlineColor(sf::Color(0, 0, 64));
        btn.rect.setOutlineThickness(2.0f);
        btn.rect.setOrigin({ m_buttonSize.x * 0.5f, m_buttonSize.y * 0.5f });
        btn.rect.setPosition({ m_buttonPos.x, m_buttonPos.y + (i * (m_buttonSize.y + m_buttonPadding)) });

        btn.label.setString(buttonNames[i]);
        btn.label.setCharacterSize(30);
        sf::FloatRect labelRect = btn.label.getLocalBounds();
        btn.label.setOrigin({ labelRect.position.x + labelRect.size.x * 0.5f,
                              labelRect.position.y + labelRect.size.y * 0.5f });
        btn.label.setPosition(btn.rect.getPosition());

        m_buttons.push_back(std::move(btn));
    }

    // Register click
    context.m_eventManager.AddCallback(StateType::MainMenu, "Left_Click", &State_MainMenu::MouseClick, *this);
}

void State_MainMenu::OnDestroy()
{
    m_stateManager.GetContext().m_eventManager.RemoveCallback(StateType::MainMenu, "Left_Click");
    m_stateManager.GetContext().m_textureManager.ReleaseResource("MenuBg");
}

void State_MainMenu::Activate() {}
void State_MainMenu::Deactivate() {}

void State_MainMenu::Update(const sf::Time& time)
{
    // Hover effect
    sf::RenderWindow& window = m_stateManager.GetContext().m_window.GetRenderWindow();
    sf::View uiView = m_stateManager.GetContext().m_window.GetUIView();
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    sf::Vector2f mouseWorldPos = window.mapPixelToCoords(mousePos, uiView);

    for (auto& btn : m_buttons) {
        if (btn.rect.getGlobalBounds().contains(mouseWorldPos))
            btn.rect.setFillColor(sf::Color(128, 0, 0, 160));
        else
            btn.rect.setFillColor(sf::Color(0, 0, 128, 160));
    }
}

void State_MainMenu::Draw()
{
    sf::RenderWindow& window = m_stateManager.GetContext().m_window.GetRenderWindow();

    window.setView(m_stateManager.GetContext().m_window.GetUIView());

    if (m_backgroundSprite)
        window.draw(*m_backgroundSprite);

    window.draw(m_title);

    for (const auto& btn : m_buttons) {
        window.draw(btn.rect);
        window.draw(btn.label);
    }
}

void State_MainMenu::MouseClick(EventDetails& details)
{
    sf::RenderWindow& window = m_stateManager.GetContext().m_window.GetRenderWindow();
    sf::View uiView = m_stateManager.GetContext().m_window.GetUIView();
    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Vector2i(details.m_mouse.x, details.m_mouse.y), uiView);

    for (size_t i = 0; i < m_buttons.size(); ++i)
    {
        if (m_buttons[i].rect.getGlobalBounds().contains(mousePos))
        {
            if (i == 0) { // PLAY
                m_stateManager.SwitchTo(StateType::Game);
            }
            else if (i == 1) { // CREDITS
                m_stateManager.SwitchTo(StateType::Credits);
            }
            else if (i == 2) { // SETTINGS
                m_stateManager.SwitchTo(StateType::Settings);
            }
            else if (i == 3) { // EXIT
                m_stateManager.GetContext().m_window.Close(details);
            }
            break;
        }
    }
}