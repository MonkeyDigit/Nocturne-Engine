#include "State_MainMenu.h"
#include "StateManager.h"
#include "SharedContext.h"
#include "Window.h"

State_MainMenu::State_MainMenu(StateManager& stateManager)
    : BaseState(stateManager), m_title(m_fontTitle), m_buttonPadding(20)
{}

State_MainMenu::~State_MainMenu() {}

void State_MainMenu::OnCreate()
{
    SharedContext& context = m_stateManager.GetContext();

    sf::Vector2f uiRes = context.m_window.GetUIResolution();

    // Load Background
    SetupCenteredBackground(
        m_backgroundSprite,
        "MenuBg",
        uiRes,
        "State_MainMenu");

    // Load fonts
    LoadFontOrWarn(m_fontTitle, "media/fonts/EightBitDragon.ttf", "State_MainMenu", "title");
    LoadFontOrWarn(m_fontButton, "media/fonts/EightBitDragon.ttf", "State_MainMenu", "buttons");

    m_title.setString("Nocturne Engine");
    m_title.setCharacterSize(80);
    m_title.setStyle(sf::Text::Bold);
    m_title.setOutlineThickness(2);
    m_title.setFillColor(sf::Color::Yellow);
    m_title.setOutlineColor(sf::Color::Red);
    CenterText(m_title, uiRes.x * 0.5f, uiRes.y * 0.2f);

    // SETUP BUTTONS
    m_buttonSize = sf::Vector2f(300.0f, 60.0f);
    m_buttonPos = sf::Vector2f(uiRes.x * 0.5f, uiRes.y * 0.4f);
    std::vector<std::string> buttonNames = { "PLAY", "CREDITS", "SETTINGS", "EXIT" };
    for (size_t i = 0; i < buttonNames.size(); ++i)
    {
        MenuButton btn(m_fontButton);

        const sf::Vector2f buttonCenter = {
            m_buttonPos.x,
            m_buttonPos.y + (static_cast<float>(i) * (m_buttonSize.y + m_buttonPadding))
        };

        SetupTextButton(
            btn.rect,
            btn.label,
            m_buttonSize,
            buttonCenter,
            buttonNames[i],
            30u);

        m_buttons.push_back(std::move(btn));
    }

    // Register click
    context.m_eventManager.AddCallback(StateType::MainMenu, "Left_Click", &State_MainMenu::MouseClick, *this);
}

void State_MainMenu::OnDestroy()
{
    m_stateManager.GetContext().m_eventManager.RemoveCallback(StateType::MainMenu, "Left_Click");
    ReleaseTrackedTextures("State_MainMenu");
}

void State_MainMenu::Activate() {}
void State_MainMenu::Deactivate() {}

void State_MainMenu::Update(const sf::Time&)
{
    // Hover effect
    sf::RenderWindow& window = m_stateManager.GetContext().m_window.GetRenderWindow();
    sf::View uiView = m_stateManager.GetContext().m_window.GetUIView();
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    sf::Vector2f mouseWorldPos = window.mapPixelToCoords(mousePos, uiView);

    for (auto& btn : m_buttons)
        UpdateButtonHoverColor(btn.rect, mouseWorldPos);
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