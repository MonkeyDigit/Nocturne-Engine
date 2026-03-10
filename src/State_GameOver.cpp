#include <cmath>
#include <cstdint>
#include "State_GameOver.h"
#include "StateManager.h"
#include "SharedContext.h"
#include "Window.h"

State_GameOver::State_GameOver(StateManager& stateManager)
    : BaseState(stateManager),
    m_title(m_fontTitle),
    m_subtitle(m_fontBody),
    m_prompt(m_fontBody),
    m_timePassed(0.0f)
{
}

State_GameOver::~State_GameOver() {}

void State_GameOver::OnCreate()
{
    SharedContext& context = m_stateManager.GetContext();
    const sf::Vector2f uiRes = context.m_window.GetUIResolution();

    SetupCenteredBackground(
        m_backgroundSprite,
        "MenuBg",
        uiRes,
        "State_GameOver");

    m_overlay.setSize(uiRes);
    m_overlay.setFillColor(sf::Color(0, 0, 0, 185));

    LoadFontOrWarn(m_fontTitle, "media/fonts/EightBitDragon.ttf", "State_GameOver", "title");
    LoadFontOrWarn(m_fontBody, "media/fonts/EightBitDragon.ttf", "State_GameOver", "body");

    m_title.setString("GAME OVER");
    m_title.setCharacterSize(86);
    m_title.setStyle(sf::Text::Bold);
    m_title.setFillColor(sf::Color(245, 90, 90));
    m_title.setOutlineColor(sf::Color::Black);
    m_title.setOutlineThickness(2.0f);
    CenterText(m_title, uiRes.x * 0.5f, uiRes.y * 0.28f);

    m_subtitle.setString("You have been defeated.");
    m_subtitle.setCharacterSize(30);
    m_subtitle.setFillColor(sf::Color::White);
    CenterText(m_subtitle, uiRes.x * 0.5f, uiRes.y * 0.48f);

    m_prompt.setString("R: RETRY    ESC: MAIN MENU");
    m_prompt.setCharacterSize(24);
    m_prompt.setFillColor(sf::Color::White);
    CenterText(m_prompt, uiRes.x * 0.5f, uiRes.y * 0.82f);

    context.m_window.GetRenderWindow().setMouseCursorVisible(true);

    context.m_eventManager.AddCallback(
        StateType::GameOver, "GameOver_Retry", &State_GameOver::Retry, *this);
    context.m_eventManager.AddCallback(
        StateType::GameOver, "GameOver_ToMainMenu", &State_GameOver::GoToMainMenu, *this);
}

void State_GameOver::OnDestroy()
{
    SharedContext& context = m_stateManager.GetContext();
    context.m_eventManager.RemoveCallback(StateType::GameOver, "GameOver_Retry");
    context.m_eventManager.RemoveCallback(StateType::GameOver, "GameOver_ToMainMenu");
    ReleaseTrackedTextures("State_GameOver");
}

void State_GameOver::Activate() {}
void State_GameOver::Deactivate() {}

void State_GameOver::Update(const sf::Time& time)
{
    m_timePassed += time.asSeconds();

    const float pulse = std::abs(std::sin(m_timePassed * 3.0f));
    const std::uint8_t alpha = static_cast<std::uint8_t>(100 + pulse * 155.0f);
    m_prompt.setFillColor(sf::Color(255, 255, 255, alpha));
}

void State_GameOver::Draw()
{
    sf::RenderWindow& window = m_stateManager.GetContext().m_window.GetRenderWindow();
    window.setView(m_stateManager.GetContext().m_window.GetUIView());

    if (m_backgroundSprite) window.draw(*m_backgroundSprite);
    window.draw(m_overlay);
    window.draw(m_title);
    window.draw(m_subtitle);
    window.draw(m_prompt);
}

void State_GameOver::Retry(EventDetails&)
{
    m_stateManager.SwitchTo(StateType::Game);
    m_stateManager.Remove(StateType::GameOver);
}

void State_GameOver::GoToMainMenu(EventDetails&)
{
    m_stateManager.SwitchTo(StateType::MainMenu);
    m_stateManager.Remove(StateType::GameOver);
}