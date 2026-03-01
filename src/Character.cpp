#include "Character.h"
#include "SharedContext.h"
#include "EntityManager.h"
#include <fstream>
#include <sstream>
#include <iostream>

Character::Character(EntityManager& entityManager)
    : EntityBase(entityManager),
    m_spriteSheet(m_entityManager.GetContext().m_textureManager),
    m_jumpVelocity(125.0f), m_maxHitPoints(5), m_hitPoints(5), m_jumpTimer(0.0f)
{
    m_name = "Character";
}

Character::~Character() {}

void Character::Move(Direction dir)
{
    if (GetState() == EntityState::Dying) return;

    if (GetState() == EntityState::Attacking && m_spriteSheet.GetCurrentAnim()->GetName() == "Attack") return;

    if (dir == Direction::Left) 
        Accelerate(-m_speed.x, 0.0f);
    else 
        Accelerate(m_speed.x, 0.0f);

    if (GetState() == EntityState::Idle) 
        SetState(EntityState::Walking);

    if (m_velocity.x < 0.0f && dir == Direction::Right) return;

    m_spriteSheet.SetDirection(dir);
}

void Character::Jump()
{
    if (GetState() == EntityState::Dying || GetState() == EntityState::Hurt) return;

    m_jumping = true;
    SetState(EntityState::Jumping);
}

void Character::Attack()
{
    if (GetState() == EntityState::Dying || GetState() == EntityState::Jumping ||
        GetState() == EntityState::Hurt || GetState() == EntityState::Attacking) return;

    SetAcceleration(0.0f, 0.0f);
    SetVelocity(0.0f, 0.0f);
    SetState(EntityState::Attacking);
}

void Character::TakeDamage(int damage)
{
    if (GetState() == EntityState::Dying || GetState() == EntityState::Hurt) return;

    m_hitPoints = (m_hitPoints - damage > 0) ? m_hitPoints - damage : 0;

    if (m_hitPoints > 0) SetState(EntityState::Hurt);
    else SetState(EntityState::Dying);
}

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

        if (type == "Name") 
            keystream >> m_name;
        else if (type == "Spritesheet")
        {
            std::string sheetPath;
            keystream >> sheetPath;
            m_spriteSheet.LoadSheet("media/spritesheets/" + sheetPath);
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
            keystream >> m_attackAABBoffset.x >> m_attackAABBoffset.y >> m_attackAABB.size.x >> m_attackAABB.size.y;
        }
        else if (type == "Speed") keystream >> m_speed.x >> m_speed.y;
        else if (type == "JumpVelocity") keystream >> m_jumpVelocity;
        else if (type == "MaxVelocity") keystream >> m_maxVelocity.x >> m_maxVelocity.y;
        else std::cerr << "! Unknown type in character file: " << type << '\n';
    }
    file.close();

    // Set a default animation
    m_spriteSheet.SetAnimation("Idle", true, true);
}

void Character::UpdateAttackAABB()
{
    // Calculating the attack hitbox based on the facing direction
    m_attackAABB.position.x = (m_spriteSheet.GetDirection() == Direction::Left ?
        (m_AABB.position.x - m_attackAABB.size.x) - m_attackAABBoffset.x
        : (m_AABB.position.x + m_AABB.size.x) + m_attackAABBoffset.x);

    m_attackAABB.position.y = m_AABB.position.y + m_attackAABBoffset.y;
}

void Character::Animate()
{
    EntityState state = GetState();

    if (state == EntityState::Walking && m_spriteSheet.GetCurrentAnim()->GetName() != "Walk")
    {
        m_spriteSheet.SetAnimation("Walk", true, true);
    }
    else if (state == EntityState::Jumping && m_spriteSheet.GetCurrentAnim()->GetName() != "Jump")
    {
        m_spriteSheet.SetAnimation("Jump", true, false);
    }
    else if (state == EntityState::Attacking && m_spriteSheet.GetCurrentAnim()->GetName() != "Attack")
    {
        m_spriteSheet.SetAnimation("Attack", true, false);
    }
    else if (state == EntityState::Hurt && m_spriteSheet.GetCurrentAnim()->GetName() != "Hurt")
    {
        m_spriteSheet.SetAnimation("Hurt", true, false);
    }
    else if (state == EntityState::Dying && m_spriteSheet.GetCurrentAnim()->GetName() != "Death")
    {
        m_spriteSheet.SetAnimation("Death", true, false);
    }
    else if (state == EntityState::Idle && m_spriteSheet.GetCurrentAnim()->GetName() != "Idle")
    {
        m_spriteSheet.SetAnimation("Idle", true, true);
    }
}

// TODO: Perché non funziona il salto con durata di pressione diverse ???
void Character::Update(float deltaTime)
{
    if (m_jumping)
    {
        // If you hit your head
        if (m_jumpTimer > 0.0f && m_velocity.y == 0.0f)
            m_jumpTimer = 1.1f;

        m_jumpTimer += deltaTime;
        if (m_jumpTimer <= 0.2f)
        {
            m_jumping = true;
            SetVelocity(m_velocity.x, -m_jumpVelocity);
        }
        else m_jumping = false;
    }
    else if (GetState() != EntityState::Jumping) 
        m_jumpTimer = 0.0f;
    else 
        m_jumpTimer = 1.1f;

    EntityBase::Update(deltaTime);

    if (m_attackAABB.size.x != 0.0f && m_attackAABB.size.y != 0.0f)
    {
        UpdateAttackAABB();
    }

    if (GetState() != EntityState::Dying && GetState() != EntityState::Attacking && GetState() != EntityState::Hurt)
    {
        if (std::abs(m_velocity.y) > 0.01f || !m_referenceTile)
        {
            SetState(EntityState::Jumping);
        }
        else if (std::abs(m_velocity.x) > 0.2f && !m_collidingOnX)
        {
            SetState(EntityState::Walking);
        }
        else
        {
            SetState(EntityState::Idle);
        }
    }
    else if (GetState() == EntityState::Attacking || GetState() == EntityState::Hurt)
    {
        if (!m_spriteSheet.GetCurrentAnim()->IsPlaying())
        {
            SetState(EntityState::Idle);
        }
    }
    else if (GetState() == EntityState::Dying)
    {
        m_entityManager.Remove(m_id);
        return;
    }

    Animate();
    m_spriteSheet.Update(deltaTime);
    m_spriteSheet.SetSpritePosition(m_position);
}

void Character::Draw(sf::RenderWindow& window)
{
    m_spriteSheet.Draw(window);
}

int Character::GetHitPoints() const { return m_hitPoints; }
int Character::GetMaxHitPoints() const { return m_maxHitPoints; }