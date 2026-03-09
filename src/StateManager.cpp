#include <algorithm>
#include <iterator>

#include "StateManager.h"
#include "SharedContext.h"
#include "Window.h"
#include "State_Intro.h"
#include "State_MainMenu.h"
#include "State_Game.h"
#include "State_Paused.h"
#include "State_Settings.h"
#include "State_Credits.h"
#include "State_Victory.h"

namespace
{
    bool IsQueuedForRemoval(const TypeContainer& queue, StateType type)
    {
        return std::find(queue.begin(), queue.end(), type) != queue.end();
    }

    template <typename OverlayPredicate>
    StateContainer::iterator ResolveStackStart(StateContainer& states, OverlayPredicate&& isOverlayState)
    {
        auto itr = std::prev(states.end());

        if (states.size() <= 1 || !isOverlayState(*itr))
            return itr;

        while (itr != states.begin())
        {
            if (!isOverlayState(*itr))
                break;
            --itr;
        }

        return itr;
    }

    template <typename Fn>
    void ForEachStateFrom(StateContainer& states, StateContainer::iterator start, Fn&& fn)
    {
        for (auto itr = start; itr != states.end(); ++itr)
            fn(*itr->second);
    }
}

StateManager::StateManager(SharedContext& shared)
    : m_shared(shared),
    m_isUpdating(false),
    m_pendingSwitch(std::nullopt)
{
    RegisterState<State_Intro>(StateType::Intro);
    RegisterState<State_MainMenu>(StateType::MainMenu);
    RegisterState<State_Game>(StateType::Game);
    RegisterState<State_Paused>(StateType::Paused);
    RegisterState<State_Settings>(StateType::Settings);
    RegisterState<State_Credits>(StateType::Credits);
    RegisterState<State_Victory>(StateType::Victory);
}

StateManager::~StateManager()
{
    // Destroy top-to-bottom to mirror stack semantics
    for (auto itr = m_states.rbegin(); itr != m_states.rend(); ++itr)
        itr->second->OnDestroy();
}

void StateManager::Draw()
{
    if (m_states.empty()) return;

    auto start = ResolveStackStart(
        m_states,
        [](const auto& statePair)
        {
            return statePair.second->IsTransparent();
        });

    ForEachStateFrom(
        m_states,
        start,
        [](BaseState& state)
        {
            state.Draw();
        });
}

void StateManager::ProcessRequests()
{
    for (StateType type : m_toRemove)
        RemoveState(type);

    m_toRemove.clear();

    if (m_pendingSwitch.has_value())
    {
        const StateType requested = *m_pendingSwitch;
        m_pendingSwitch.reset();
        SwitchTo(requested); // safe here: not inside Update iteration
    }
}

SharedContext& StateManager::GetContext() { return m_shared; }

bool StateManager::HasState(StateType type) const
{
    const auto stateIt = std::find_if(
        m_states.begin(),
        m_states.end(),
        [type](const auto& pair) { return pair.first == type; });

    if (stateIt == m_states.end())
        return false;

    return !IsQueuedForRemoval(m_toRemove, type);
}

void StateManager::SwitchTo(StateType type)
{
    // Never mutate the state stack while Update() is iterating it
    if (m_isUpdating)
    {
        m_pendingSwitch = type;
        return;
    }

    if (!m_states.empty() && m_states.back().first == type)
    {
        m_shared.m_eventManager.SetCurrentState(type);
        return;
    }

    const auto existingIt = std::find_if(
        m_states.begin(),
        m_states.end(),
        [type](const auto& pair) { return pair.first == type; });

    if (existingIt != m_states.end())
    {
        m_states.back().second->Deactivate();

        StateType tmpType = existingIt->first;
        std::unique_ptr<BaseState> tmpState = std::move(existingIt->second);
        m_states.erase(existingIt);
        m_states.emplace_back(tmpType, std::move(tmpState));

        m_states.back().second->Activate();
        m_shared.m_eventManager.SetCurrentState(type);
        return;
    }

    BaseState* previousTop = m_states.empty() ? nullptr : m_states.back().second.get();
    if (previousTop) previousTop->Deactivate();

    if (!CreateState(type))
    {
        // Restore previous state if requested state cannot be created
        if (previousTop) previousTop->Activate();
        return;
    }

    m_states.back().second->Activate();
    m_shared.m_eventManager.SetCurrentState(type);
}

void StateManager::Remove(StateType type)
{
    if (!IsQueuedForRemoval(m_toRemove, type))
        m_toRemove.push_back(type);
}

bool StateManager::CreateState(StateType type)
{
    auto factory = m_stateFactory.find(type);
    if (factory == m_stateFactory.end()) return false;

    std::unique_ptr<BaseState> state = factory->second();
    if (!state) return false;

    m_states.emplace_back(type, std::move(state));
    m_states.back().second->OnCreate();
    return true;
}

void StateManager::RemoveState(StateType type)
{
    const auto itr = std::find_if(
        m_states.begin(),
        m_states.end(),
        [type](const auto& pair) { return pair.first == type; });

    if (itr == m_states.end())
        return;

    const bool wasTopState = (itr == std::prev(m_states.end()));

    itr->second->OnDestroy();
    m_states.erase(itr);

    // If we removed the active state, activate the new top state
    if (wasTopState && !m_states.empty())
    {
        m_shared.m_eventManager.SetCurrentState(m_states.back().first);
        m_states.back().second->Activate();
    }
}

void StateManager::Update(const sf::Time& time)
{
    if (m_states.empty()) return;

    m_isUpdating = true;

    auto start = ResolveStackStart(
        m_states,
        [](const auto& statePair)
        {
            return statePair.second->IsTranscendent();
        });

    ForEachStateFrom(
        m_states,
        start,
        [&time](BaseState& state)
        {
            state.Update(time);
        });

    m_isUpdating = false;
}