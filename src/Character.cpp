#include <fstream>
#include <sstream>
#include <iostream>
#include "Character.h"
#include "SharedContext.h"
#include "EntityManager.h"
#include "CState.h"
#include "CController.h"

Character::Character(EntityManager& entityManager)
    : EntityBase(entityManager)
{
    m_name = "Character";
    TextureManager& textures = entityManager.GetContext().m_textureManager;
    m_sprite = AddComponent<CSprite>(textures);
}

Character::~Character() {}

void Character::Load(const std::string& path)
{
    std::ifstream file{ Utils::GetWorkingDirectory() + path };
    if (!file.is_open())
    {
        std::cerr << "! Failed loading character file: " << path << '\n';
        return;
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '|') continue;

        std::stringstream keystream{ line };
        std::string type;

        keystream >> type;

        // TODO: Spostare ?
        if (type == "Name") 
            keystream >> m_name;
        else if (type == "Spritesheet")
        {
            std::string sheetPath;
            keystream >> sheetPath;
            m_sprite->Load("media/spritesheets/" + sheetPath);
        }
        else if (type == "Hitpoints")
        {
            int maxHitPoints;
            keystream >> maxHitPoints;
            this->GetComponent<CState>()->SetHitPoints(maxHitPoints);
        }
        else if (type == "BoundingBox")
        {
            float x, y;
            keystream >> x >> y;
            SetSize(x, y);
        }
        else if (type == "DamageBox")
        {
            keystream >> m_attackAABBoffset.x >> m_attackAABBoffset.y >> m_attackAABB.size.x >> m_attackAABB.size.y;
        }
        else if (type == "Speed")
        {
            float x, y;
            keystream >> x >> y;
            SetSpeed(x, y);
        }
        else if (type == "MaxVelocity")
        {
            float x, y;
            keystream >> x >> y;
            SetMaxVelocity(x, y);
        }
        else if (type == "JumpVelocity")
        {
            float jv;
            keystream >> jv;

            CController* controller = GetComponent<CController>();
            if (controller)
                controller->m_jumpVelocity = jv;
        }
        else
            std::cerr << "! Unknown type in character file: " << type << '\n';

    }
    file.close();

    // Set a default animation
    m_sprite->GetSpriteSheet().SetAnimation("Idle", true, true);
}

void Character::UpdateAttackAABB()
{
    const sf::FloatRect& aabb = m_collider->GetAABB();
    // Calculating the attack hitbox based on the facing direction
    m_attackAABB.position.x = (m_sprite->GetSpriteSheet().GetDirection() == Direction::Left ?
        (aabb.position.x - m_attackAABB.size.x) - m_attackAABBoffset.x
        : (aabb.position.x + aabb.size.x) + m_attackAABBoffset.x);

    m_attackAABB.position.y = aabb.position.y + m_attackAABBoffset.y;
}

void Character::Update(float deltaTime)
{
    EntityBase::Update(deltaTime);

    if (m_attackAABB.size.x != 0.0f && m_attackAABB.size.y != 0.0f)
    {
        UpdateAttackAABB();
    }

    if (this->GetComponent<CState>()->GetState() != EntityState::Dying && this->GetComponent<CState>()->GetState() != EntityState::Attacking && this->GetComponent<CState>()->GetState() != EntityState::Hurt)
    {
        if (std::abs(GetVelocity().y) > 0.01f || !m_collider->GetReferenceTile())
        {
            this->GetComponent<CState>()->SetState(EntityState::Jumping);
        }
        else if (std::abs(GetVelocity().x) > 0.2f && !m_collider->IsCollidingX())
        {
            this->GetComponent<CState>()->SetState(EntityState::Walking);
        }
        else
        {
            this->GetComponent<CState>()->SetState(EntityState::Idle);
        }
    }
    else if (this->GetComponent<CState>()->GetState() == EntityState::Attacking || this->GetComponent<CState>()->GetState() == EntityState::Hurt)
    {
        if (!m_sprite->GetSpriteSheet().GetCurrentAnim()->IsPlaying())
        {
            this->GetComponent<CState>()->SetState(EntityState::Idle);
        }
    }
    else if (this->GetComponent<CState>()->GetState() == EntityState::Dying)
    {
        m_entityManager.Remove(m_id);
        return;
    }

    // TODO: Mettere qualcosa qua?
    m_sprite->GetSpriteSheet().SetSpritePosition(GetPosition());
}