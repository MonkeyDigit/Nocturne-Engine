#include "StateManager.h"
#include "SharedContext.h"
#include "Window.h"
// TODO: Uncomment these as we refactor each state
// #include "State_Intro.h"
#include "State_MainMenu.h"
// #include "State_Settings.h"
#include "State_Game.h"
#include "State_Paused.h"
// #include "State_Credits.h"

StateManager::StateManager(SharedContext& shared)
    : m_shared(shared)
{
    // TODO: Uncomment these as we recreate the specific states
    // RegisterState<State_Intro>(StateType::Intro);
    RegisterState<State_MainMenu>(StateType::MainMenu);
    // RegisterState<State_Settings>(StateType::Settings);
    RegisterState<State_Game>(StateType::Game);
    RegisterState<State_Paused>(StateType::Paused);
    // RegisterState<State_Credits>(StateType::Credits);
}

StateManager::~StateManager()
{
    // unique_ptr will handle memory deletion, but we still need to trigger OnDestroy manually
    for (auto& statePair : m_states)
    {
        statePair.second->OnDestroy();
    }
}

void StateManager::Draw()
{
    if (m_states.empty()) return;

    bool adjustView = false;
    if (m_windSize != m_shared.m_window.GetWindowSize())
    {
        m_windSize = m_shared.m_window.GetWindowSize();
        adjustView = true;
    }

    // Check if the top state is transparent (e.g., Paused state over Game state)
    if (m_states.back().second->IsTransparent() && m_states.size() > 1)
    {
        auto itr = m_states.end();
        --itr;
        while (itr != m_states.begin())
        {
            if (!itr->second->IsTransparent()) break;
            --itr;
        }

        for (; itr != m_states.end(); ++itr)
        {
            if (adjustView) itr->second->AdjustView();
            itr->second->Draw();
        }
    }
    else
    {
        if (adjustView) m_states.back().second->AdjustView();
        m_states.back().second->Draw();
    }
}

void StateManager::ProcessRequests()
{
    while (!m_toRemove.empty())
    {
        RemoveState(*m_toRemove.begin());
        m_toRemove.erase(m_toRemove.begin());
    }
}

SharedContext& StateManager::GetContext() { return m_shared; }

bool StateManager::HasState(StateType type) const
{
    for (auto itr = m_states.begin(); itr != m_states.end(); ++itr)
    {
        if (itr->first == type)
        {
            auto removed = std::find(m_toRemove.begin(), m_toRemove.end(), type);
            return removed == m_toRemove.end();
        }
    }
    return false;
}

void StateManager::SwitchTo(StateType type)
{
    m_shared.m_eventManager.SetCurrentState(type);

    for (auto itr = m_states.begin(); itr != m_states.end(); ++itr)
    {
        if (itr->first == type)
        {
            m_states.back().second->Deactivate();

            // Move the found state to the back of the stack
            StateType tmpType = itr->first;
            std::unique_ptr<BaseState> tmpState = std::move(itr->second);
            m_states.erase(itr);
            m_states.emplace_back(tmpType, std::move(tmpState));

            m_states.back().second->Activate();
            m_states.back().second->AdjustView();
            return;
        }
    }

    // State wasn't found, we need to create it
    if (!m_states.empty()) m_states.back().second->Deactivate();

    CreateState(type);

    if (!m_states.empty())
    {
        m_states.back().second->Activate();
        m_states.back().second->AdjustView();
    }
}

void StateManager::Remove(StateType type)
{
    m_toRemove.push_back(type);
}

void StateManager::CreateState(StateType type)
{
    auto factory = m_stateFactory.find(type);
    if (factory == m_stateFactory.end()) return;

    std::unique_ptr<BaseState> state = factory->second();
    state->SetView(m_shared.m_window.GetRenderWindow().getDefaultView());

    m_states.emplace_back(type, std::move(state));
    m_states.back().second->OnCreate();
}

void StateManager::RemoveState(StateType type)
{
    for (auto itr = m_states.begin(); itr != m_states.end(); ++itr)
    {
        if (itr->first == type)
        {
            itr->second->OnDestroy();
            m_states.erase(itr); // The unique_ptr automatically frees the memory here
            return;
        }
    }
}

void StateManager::Update(const sf::Time& time)
{
    if (m_states.empty()) return;

    if (m_states.back().second->IsTranscendent() && m_states.size() > 1)
    {
        auto itr = m_states.end();
        --itr;
        while (itr != m_states.begin())
        {
            if (!itr->second->IsTranscendent()) break;
            --itr;
        }

        for (; itr != m_states.end(); ++itr)
        {
            itr->second->Update(time);
        }
    }
    else
    {
        m_states.back().second->Update(time);
    }
}