#include <cmath>
#include <cstdint>
#include "State_Victory.h"
#include "StateManager.h"
#include "SharedContext.h"
#include "Window.h"

State_Victory::State_Victory(StateManager& stateManager)
    : BaseState(stateManager),
    m_title(m_fontTitle),
    m_subtitle(m_fontBody),
    m_prompt(m_fontBody),
    m_timePassed(0.0f)
{
}

State_Victory::~State_Victory() {}

void State_Victory::OnCreate()
{
    SharedContext& context = m_stateManager.GetContext();
    const sf::Vector2f uiRes = context.m_window.GetUIResolution();

    SetupCenteredBackground(
        m_backgroundSprite,
        "MenuBg",
        uiRes,
        "State_Victory");

    m_overlay.setSize(uiRes);
    m_overlay.setFillColor(sf::Color(0, 0, 0, 170));

    LoadFontOrWarn(m_fontTitle, "media/fonts/EightBitDragon.ttf", "State_Victory", "title");
    LoadFontOrWarn(m_fontBody, "media/fonts/EightBitDragon.ttf", "State_Victory", "body");

    m_title.setString("VICTORY");
    m_title.setCharacterSize(90);
    m_title.setStyle(sf::Text::Bold);
    m_title.setFillColor(sf::Color::Yellow);
    m_title.setOutlineColor(sf::Color::Red);
    m_title.setOutlineThickness(2.0f);
    CenterText(m_title, uiRes.x * 0.5f, uiRes.y * 0.28f);

    m_subtitle.setString("You cleared the current vertical slice.");
    m_subtitle.setCharacterSize(30);
    m_subtitle.setFillColor(sf::Color::White);
    CenterText(m_subtitle, uiRes.x * 0.5f, uiRes.y * 0.48f);

    m_prompt.setString("SPACE: CREDITS    ESC: MAIN MENU");
    m_prompt.setCharacterSize(24);
    m_prompt.setFillColor(sf::Color::White);
    CenterText(m_prompt, uiRes.x * 0.5f, uiRes.y * 0.82f);

    context.m_eventManager.AddCallback(
        StateType::Victory, "Victory_ToMainMenu", &State_Victory::GoToMainMenu, *this);
    context.m_eventManager.AddCallback(
        StateType::Victory, "Victory_ToCredits", &State_Victory::GoToCredits, *this);
}

void State_Victory::OnDestroy()
{
    SharedContext& context = m_stateManager.GetContext();
    context.m_eventManager.RemoveCallback(StateType::Victory, "Victory_ToMainMenu");
    context.m_eventManager.RemoveCallback(StateType::Victory, "Victory_ToCredits");
    ReleaseTrackedTextures("State_Victory");
}

void State_Victory::Activate() {}
void State_Victory::Deactivate() {}

void State_Victory::Update(const sf::Time& time)
{
    m_timePassed += time.asSeconds();

    const float pulse = std::abs(std::sin(m_timePassed * 3.0f));
    const std::uint8_t alpha = static_cast<std::uint8_t>(100 + pulse * 155.0f);
    m_prompt.setFillColor(sf::Color(255, 255, 255, alpha));
}

void State_Victory::Draw()
{
    sf::RenderWindow& window = m_stateManager.GetContext().m_window.GetRenderWindow();
    window.setView(m_stateManager.GetContext().m_window.GetUIView());

    if (m_backgroundSprite) window.draw(*m_backgroundSprite);
    window.draw(m_overlay);
    window.draw(m_title);
    window.draw(m_subtitle);
    window.draw(m_prompt);
}

void State_Victory::GoToMainMenu(EventDetails&)
{
    m_stateManager.SwitchTo(StateType::MainMenu);
    m_stateManager.Remove(StateType::Victory);
}

void State_Victory::GoToCredits(EventDetails&)
{
    m_stateManager.SwitchTo(StateType::Credits);
    m_stateManager.Remove(StateType::Victory);
}