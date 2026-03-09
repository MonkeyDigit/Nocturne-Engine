#include "MovementControlSystem.h"
#include "EntityManager.h"
#include "SharedContext.h"
#include "CController.h"
#include "StateManager.h"

void MovementControlSystem::Initialize(EntityManager* entityManager)
{
    m_entityManager = entityManager;
    if (!m_entityManager) return;

    EventManager& events = m_entityManager->GetContext().m_eventManager;

    events.AddCallback(StateType::Game, "Player_MoveLeft", &MovementControlSystem::React, *this);
    events.AddCallback(StateType::Game, "Player_MoveRight", &MovementControlSystem::React, *this);
    events.AddCallback(StateType::Game, "Player_Jump", &MovementControlSystem::React, *this);
    events.AddCallback(StateType::Game, "Player_Jump_Cancel", &MovementControlSystem::React, *this);
    events.AddCallback(StateType::Game, "Player_Attack", &MovementControlSystem::React, *this);
    events.AddCallback(StateType::Game, "Attack_Ranged", &MovementControlSystem::React, *this);
}

void MovementControlSystem::Destroy()
{
    if (!m_entityManager) return;
    EventManager& events = m_entityManager->GetContext().m_eventManager;

    events.RemoveCallback(StateType::Game, "Player_MoveLeft");
    events.RemoveCallback(StateType::Game, "Player_MoveRight");
    events.RemoveCallback(StateType::Game, "Player_Jump");
    events.RemoveCallback(StateType::Game, "Player_Jump_Cancel");
    events.RemoveCallback(StateType::Game, "Player_Attack");
    events.RemoveCallback(StateType::Game, "Attack_Ranged");
}

void MovementControlSystem::React(EventDetails& details)
{
    if (!m_entityManager) return;

    // This input path is intentionally single-player
    EntityBase* player = m_entityManager->GetPlayer();
    if (!player) return;

    CController* controller = player->GetComponent<CController>();
    if (!controller) return;

    const std::string& action = details.m_name;

    if (action == "Player_MoveLeft") controller->m_moveLeft = true;
    else if (action == "Player_MoveRight") controller->m_moveRight = true;
    else if (action == "Player_Jump") controller->m_jump = true;
    else if (action == "Player_Jump_Cancel") controller->m_cancelJump = true;
    else if (action == "Player_Attack") controller->m_attack = true;
    else if (action == "Attack_Ranged" && controller->m_rangedEnabled) controller->m_attackRanged = true;
}