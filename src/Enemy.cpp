#include <cmath>
#include <cstdlib>
#include "Enemy.h"
#include "EntityManager.h"
#include "CState.h"
#include "CController.h"
#include "CAIPatrol.h"

Enemy::Enemy(EntityManager& entityManager)
    : Character(entityManager)
{
    m_type = EntityType::Enemy;
    AddComponent<CController>();
    AddComponent<CAIPatrol>();
}