#include "BaseState.h"
#include "StateManager.h"
#include "SharedContext.h"
#include "Window.h"

BaseState::BaseState(StateManager& stateManager)
    : m_stateManager(stateManager),
    m_transparent(false),
    m_transcendent(false)
{
}

void BaseState::SetTransparent(bool transparent) { m_transparent = transparent; }
void BaseState::SetTranscendent(bool transcendence) { m_transcendent = transcendence; }

bool BaseState::IsTransparent() const { return m_transparent; }
bool BaseState::IsTranscendent() const { return m_transcendent; }

StateManager& BaseState::GetStateManager() { return m_stateManager; }