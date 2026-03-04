#include "Player.h"
#include "EntityManager.h"
#include "CController.h"
#include "CState.h"

Player::Player(EntityManager& entityManager)
    : Character(entityManager)
{
    m_type = EntityType::Player;
    // IMPORTANT: Add Controller Component first, then load character data
    AddComponent<CController>();
    Load("media/characters/Player.char");
}