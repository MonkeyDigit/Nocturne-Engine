#include "MovementControlSystem.h"
#include "EntityManager.h"
#include "SharedContext.h"
#include "CController.h"
#include "CTransform.h"
#include "CState.h"
#include "CSprite.h"
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
    events.AddCallback(StateType::Game, "Attack_Ranged", &MovementControlSystem::Player_AttackRanged, *this);
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
    std::string action = details.m_name;

    // Route the input to any entity that is a Player and has a Controller
    for (auto& entityPair : m_entityManager->GetEntities())
    {
        EntityBase* entity = entityPair.second.get();

        if (entity->GetType() == EntityType::Player)
        {
            CController* controller = entity->GetComponent<CController>();
            if (!controller) continue;

            if (action == "Player_MoveLeft")            controller->m_moveLeft = true;
            else if (action == "Player_MoveRight")      controller->m_moveRight = true;
            else if (action == "Player_Jump")           controller->m_jump = true;
            else if (action == "Player_Jump_Cancel")    controller->m_cancelJump = true;
            else if (action == "Player_Attack")         controller->m_attack = true;
        }
    }
}

void MovementControlSystem::Player_AttackRanged(EventDetails& details)
{
    EntityBase* player = m_entityManager->Find("Player");
    if (!player) return;

    CTransform* transform = player->GetComponent<CTransform>();
    CSprite* sprite = player->GetComponent<CSprite>();
    if (!transform || !sprite) return;

    // Calcoliamo da dove parte la palla di fuoco e in che direzione va
    sf::Vector2f startPos = transform->GetPosition();
    sf::Vector2f velocity(300.0f, 0.0f); // Velocità base verso destra

    // Se il giocatore è girato a sinistra, invertiamo la velocità!
    if (sprite->GetSpriteSheet().GetDirection() == Direction::Left)
    {
        velocity.x = -300.0f;
    }

    // Spawniamo il proiettile! (10 danni, 1.5 secondi di vita)
    m_entityManager->SpawnProjectile(player, startPos, velocity, 10, 1.5f);
}

void MovementControlSystem::Update(float deltaTime)
{
    if (!m_entityManager) return;

    for (auto& entityPair : m_entityManager->GetEntities())
    {
        EntityBase* entity = entityPair.second.get();
        CController* controller = entity->GetComponent<CController>();
        CTransform* transform = entity->GetComponent<CTransform>();
        CState* state = entity->GetComponent<CState>();
        CSprite* sprite = entity->GetComponent<CSprite>();

        // Only process controllable entities
        if (!controller || !transform || !state) continue;

        EntityState currentState = state->GetState();
        if (currentState == EntityState::Dying) continue;

        // TIMERS (Coyote & Buffer)
        if (currentState != EntityState::Jumping) controller->m_coyoteTimer = 0.1f;
        else controller->m_coyoteTimer -= deltaTime;

        controller->m_jumpBufferTimer -= deltaTime;

        // MOVEMENT
        if (currentState != EntityState::Attacking && currentState != EntityState::Hurt)
        {
            if (controller->m_moveLeft)
            {
                if (sprite) sprite->GetSpriteSheet().SetDirection(Direction::Left);
                transform->AddAcceleration(-transform->GetSpeed().x, 0.0f);
                if (currentState == EntityState::Idle) state->SetState(EntityState::Walking);
            }
            else if (controller->m_moveRight)
            {
                if (sprite) sprite->GetSpriteSheet().SetDirection(Direction::Right);
                transform->AddAcceleration(transform->GetSpeed().x, 0.0f);
                if (currentState == EntityState::Idle) state->SetState(EntityState::Walking);
            }
        }

        // JUMP
        if (controller->m_jump)
        {
            if (currentState != EntityState::Hurt && currentState != EntityState::Jumping)
            {
                controller->m_jumpBufferTimer = 0.15f;
            }
            controller->m_jump = false; // Consume the input
        }

        if (controller->m_jumpBufferTimer > 0.0f && controller->m_coyoteTimer > 0.0f)
        {
            controller->m_jumpBufferTimer = 0.0f;
            controller->m_coyoteTimer = 0.0f;
            state->SetState(EntityState::Jumping);
            transform->SetVelocity(transform->GetVelocity().x, -controller->m_jumpVelocity);
        }

        if (controller->m_cancelJump)
        {
            if (transform->GetVelocity().y < 0.0f)
                transform->SetVelocity(transform->GetVelocity().x, transform->GetVelocity().y * 0.5f);
            controller->m_cancelJump = false; // Consume the input
        }

        // ATTACK
        if (controller->m_attack)
        {
            if (currentState != EntityState::Jumping && currentState != EntityState::Hurt && currentState != EntityState::Attacking)
            {
                transform->SetAcceleration(0.0f, 0.0f);
                transform->SetVelocity(0.0f, 0.0f);
                state->SetState(EntityState::Attacking);
            }
            controller->m_attack = false; // Consume the input
        }

        // STATE CLEANUP (Idle detection)
        EntityState updatedState = state->GetState();

        if (updatedState != EntityState::Dying && updatedState != EntityState::Attacking && updatedState != EntityState::Hurt)
        {
            if (std::abs(transform->GetVelocity().y) > 0.01f) state->SetState(EntityState::Jumping);
            else if (std::abs(transform->GetVelocity().x) > 0.2f) state->SetState(EntityState::Walking);
            else state->SetState(EntityState::Idle);
        }

        // Reset continuous movement flags
        controller->m_moveLeft = false;
        controller->m_moveRight = false;
    }
}