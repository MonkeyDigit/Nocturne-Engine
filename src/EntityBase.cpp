#include <cmath>
#include <algorithm>
#include <iostream>
#include "EntityBase.h"
#include "SharedContext.h"
#include "EntityManager.h"
#include "CState.h"

bool SortCollisions(const CollisionElement& e1, const CollisionElement& e2)
{
    return e1.m_area > e2.m_area;
}

EntityBase::EntityBase(EntityManager& entityManager)
    : m_entityManager(entityManager), m_name("BaseEntity"),
    m_type(EntityType::Base),m_id(0)
{
    // Add the core components and store their shortcuts
    m_transform = AddComponent<CTransform>();
    m_collider = AddComponent<CBoxCollider>();
    // TODO: Assegnare a qualcosa?
    AddComponent<CState>();
}

EntityBase::~EntityBase() {}

void EntityBase::Update(float deltaTime)
{
    // TODO: Mettere qualcosa
}

EntityType EntityBase::GetType() const { return m_type; }
std::string EntityBase::GetName() const { return m_name; }
unsigned int EntityBase::GetId() const { return m_id; }