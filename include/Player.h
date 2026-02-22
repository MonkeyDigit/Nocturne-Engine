#pragma once
#include "Character.h"
#include "EventManager.h"

class Player : public Character
{
public:
    Player(EntityManager& entityManager);
    ~Player() override;

    void OnEntityCollision(EntityBase* collider, bool attack) override;
    void Update(float deltaTime) override;

    // --- EVENT CALLBACKS ---
    // TODO: Rimettere react to input?
    void MoveLeft(EventDetails& details);
    void MoveRight(EventDetails& details);
    void Jump(EventDetails& details);
    void Attack(EventDetails& details);

private:
    // Advanced Platformer mechanics
    float m_coyoteTimer;
    float m_jumpBufferTimer;

    const float COYOTE_TIME = 0.15f;
    const float JUMP_BUFFER_TIME = 0.1f;
    const float JUMP_CUT_MULTIPLIER = 0.5f;
};