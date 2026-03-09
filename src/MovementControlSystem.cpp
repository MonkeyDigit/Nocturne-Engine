#include <array>

#include "MovementControlSystem.h"
#include "EntityManager.h"
#include "SharedContext.h"
#include "CController.h"
#include "StateManager.h"

namespace
{
    // Keep action names centralized to avoid drift between Initialize/Destroy/React.
    constexpr const char* kActionMoveLeft = "Player_MoveLeft";
    constexpr const char* kActionMoveRight = "Player_MoveRight";
    constexpr const char* kActionJump = "Player_Jump";
    constexpr const char* kActionJumpCancel = "Player_Jump_Cancel";
    constexpr const char* kActionAttack = "Player_Attack";
    constexpr const char* kActionAttackRanged = "Attack_Ranged";

    constexpr std::array<const char*, 6> kGameInputActions = {
        kActionMoveLeft,
        kActionMoveRight,
        kActionJump,
        kActionJumpCancel,
        kActionAttack,
        kActionAttackRanged
    };

    enum class PlayerAction
    {
        None,
        MoveLeft,
        MoveRight,
        Jump,
        JumpCancel,
        Attack,
        AttackRanged
    };

    PlayerAction ResolvePlayerAction(const std::string& actionName)
    {
        if (actionName == kActionMoveLeft) return PlayerAction::MoveLeft;
        if (actionName == kActionMoveRight) return PlayerAction::MoveRight;
        if (actionName == kActionJump) return PlayerAction::Jump;
        if (actionName == kActionJumpCancel) return PlayerAction::JumpCancel;
        if (actionName == kActionAttack) return PlayerAction::Attack;
        if (actionName == kActionAttackRanged) return PlayerAction::AttackRanged;
        return PlayerAction::None;
    }

    void RegisterGameCallbacks(EventManager& events, MovementControlSystem& system)
    {
        for (const char* actionName : kGameInputActions)
            events.AddCallback(StateType::Game, actionName, &MovementControlSystem::React, system);
    }

    void UnregisterGameCallbacks(EventManager& events)
    {
        for (const char* actionName : kGameInputActions)
            events.RemoveCallback(StateType::Game, actionName);
    }
}

void MovementControlSystem::Initialize(EntityManager* entityManager)
{
    if (m_entityManager && m_entityManager != entityManager)
        Destroy();

    m_entityManager = entityManager;
    if (!m_entityManager) return;

    EventManager& events = m_entityManager->GetContext().m_eventManager;
    RegisterGameCallbacks(events, *this);
}

void MovementControlSystem::Destroy()
{
    if (!m_entityManager) return;

    EventManager& events = m_entityManager->GetContext().m_eventManager;
    UnregisterGameCallbacks(events);

    // Prevent stale pointer usage after teardown.
    m_entityManager = nullptr;
}

void MovementControlSystem::React(EventDetails& details)
{
    if (!m_entityManager) return;

    // This input path is intentionally single-player.
    EntityBase* player = m_entityManager->GetPlayer();
    if (!player) return;

    CController* controller = player->GetComponent<CController>();
    if (!controller) return;

    switch (ResolvePlayerAction(details.m_name))
    {
    case PlayerAction::MoveLeft:
        controller->m_moveLeft = true;
        break;
    case PlayerAction::MoveRight:
        controller->m_moveRight = true;
        break;
    case PlayerAction::Jump:
        controller->m_jump = true;
        break;
    case PlayerAction::JumpCancel:
        controller->m_cancelJump = true;
        break;
    case PlayerAction::Attack:
        controller->m_attack = true;
        break;
    case PlayerAction::AttackRanged:
        if (controller->m_rangedEnabled)
            controller->m_attackRanged = true;
        break;
    case PlayerAction::None:
    default:
        break;
    }
}