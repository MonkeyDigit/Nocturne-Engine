#include <fstream>
#include <sstream>
#include <iostream>
#include "Character.h"
#include "EntityManager.h"
#include "SharedContext.h"
#include "Utilities.h"

Character::Character(EntityManager& entityManager)
    : EntityBase(entityManager),
    m_spriteSheet(entityManager.GetContext().m_textureManager),
    m_jumpVelocity(0.0f),
    m_hitPoints(5),
    m_maxHitPoints(5),
    m_invulnerabilityTimer(0.0f)
{}

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

        std::stringstream keystream(line);
        std::string type;
        keystream >> type;

        if (type == "Name")
        {
            keystream >> m_name;
        }
        else if (type == "Spritesheet")
        {
            std::string sheetPath;
            keystream >> sheetPath;
            m_spriteSheet.LoadSheet("media/spritesheets/"+sheetPath);
        }
        else if (type == "Hitpoints")
        {
            keystream >> m_maxHitPoints;
            m_hitPoints = m_maxHitPoints;
        }
        else if (type == "BoundingBox")
        {
            float x, y;
            keystream >> x >> y;
            SetSize(x, y);
        }
        else if (type == "DamageBox")
        {
            float offsetX, offsetY, width, height;
            keystream >> offsetX >> offsetY >> width >> height;
            m_attackAABBoffset = sf::Vector2f(offsetX, offsetY);
            m_attackAABB.size = sf::Vector2f(width, height);
        }
        else if (type == "Speed")
        {
            keystream >> m_acceleration.x >> m_friction.x;
        }
        else if (type == "JumpVelocity")
        {
            keystream >> m_jumpVelocity;
        }
        else if (type == "MaxVelocity")
        {
            keystream >> m_maxVelocity.x >> m_maxVelocity.y;
        }
    }
    file.close();
}

void Character::Update(float deltaTime)
{
    EntityBase::Update(deltaTime);

    if (m_invulnerabilityTimer > 0.0f)
    {
        m_invulnerabilityTimer -= deltaTime;
    }

    UpdateAttackAABB();
    m_spriteSheet.Update(deltaTime);
    m_spriteSheet.SetSpritePosition(m_position);
    Animate();
}

void Character::Draw(sf::RenderWindow& window)
{
    // Simple flicker for invulnerability
    if (m_invulnerabilityTimer > 0.0f && std::fmod(m_invulnerabilityTimer, 0.1f) > 0.05f)
    {
        // Don't draw to simulate flickering
    }
    else
    {
        m_spriteSheet.Draw(window);
    }
}

void Character::UpdateAttackAABB()
{
    // Orient the attack box based on the facing direction
    if (m_spriteSheet.GetDirection() == Direction::Right)
    {
        m_attackAABB.position.x = m_position.x + m_attackAABBoffset.x;
    }
    else
    {
        m_attackAABB.position.x = m_position.x - m_attackAABBoffset.x - m_attackAABB.size.x;
    }
    m_attackAABB.position.y = m_position.y - m_attackAABBoffset.y;
}

void Character::Animate()
{
    // Default fallback animations mapping
    if (m_state == EntityState::Idle) m_spriteSheet.SetAnimation("Idle", true, true);
    else if (m_state == EntityState::Walking) m_spriteSheet.SetAnimation("Walk", true, true);
    else if (m_state == EntityState::Jumping) m_spriteSheet.SetAnimation("Jump", true, false);
    else if (m_state == EntityState::Attacking) m_spriteSheet.SetAnimation("Attack", true, false);
}

int Character::GetHitPoints() const { return m_hitPoints; }
int Character::GetMaxHitPoints() const { return m_maxHitPoints; }

void Character::TakeDamage(int damage)
{
    if (m_invulnerabilityTimer > 0.0f) return;

    m_hitPoints -= damage;
    m_invulnerabilityTimer = 0.5f; // Hardcoded half-second invulnerability

    if (m_hitPoints < 0) m_hitPoints = 0;
}