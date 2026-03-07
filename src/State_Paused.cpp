#include <iostream>
#include "State_Paused.h"
#include "StateManager.h"
#include "SharedContext.h"
#include "Window.h"

State_Paused::State_Paused(StateManager& stateManager)
    : BaseState(stateManager),
    m_text(m_font)
{
}

State_Paused::~State_Paused() {}

void State_Paused::OnCreate()
{
    sf::Vector2f uiRes = m_stateManager.GetContext().m_window.GetUIResolution();

    SetTransparent(true);

    if (!m_font.openFromFile("media/fonts/EightBitDragon.ttf"))
        std::cerr << "! Failed to load font for Paused State\n";

    m_text.setString("PAUSED");
    m_text.setCharacterSize(40);
    m_text.setStyle(sf::Text::Bold);

    sf::FloatRect textRect = m_text.getLocalBounds();
    m_text.setOrigin({ textRect.position.x + textRect.size.x * 0.5f,
                       textRect.position.y + textRect.size.y * 0.5f });
    m_text.setPosition(uiRes * 0.5f);

    m_rect.setSize(uiRes);
    m_rect.setPosition({ 0.0f, 0.0f });
    m_rect.setFillColor(sf::Color(0, 0, 0, 150));

    EventManager& evMgr = m_stateManager.GetContext().m_eventManager;
    evMgr.AddCallback(StateType::Paused, "Pause", &State_Paused::Unpause, *this);
}

void State_Paused::OnDestroy()
{
    EventManager& evMgr = m_stateManager.GetContext().m_eventManager;
    evMgr.RemoveCallback(StateType::Paused, "Pause");
}

void State_Paused::Activate() {}
void State_Paused::Deactivate() {}
void State_Paused::Update(const sf::Time& time) {}

void State_Paused::Draw()
{
    sf::RenderWindow& window = m_stateManager.GetContext().m_window.GetRenderWindow();
    window.setView(m_stateManager.GetContext().m_window.GetUIView());
    window.draw(m_rect);
    window.draw(m_text);
}

void State_Paused::Unpause(EventDetails& details)
{
    m_stateManager.SwitchTo(StateType::Game);
}