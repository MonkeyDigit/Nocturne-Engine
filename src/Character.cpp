#include "Character.h"
#include "SharedContext.h"
#include "EntityManager.h"
#include <fstream>
#include <sstream>
#include <iostream>

Character::Character(EntityManager& entityManager)
    : EntityBase(entityManager),
    m_jumpVelocity(125.0f), m_maxHitPoints(5), m_hitPoints(5)
{
    m_name = "Character";
    TextureManager& textures = entityManager.GetContext().m_textureManager;
    m_sprite = AddComponent<CSprite>(textures);
}

Character::~Character() {}

void Character::Move(Direction dir)
{
    SpriteSheet& spriteSheet = m_sprite->GetSpriteSheet();

    if (GetState() == EntityState::Dying) return;

    if (GetState() == EntityState::Attacking && spriteSheet.GetCurrentAnim()->GetName() == "Attack") return;

    spriteSheet.SetDirection(dir);

    if (dir == Direction::Left) 
        AddAcceleration(-GetSpeed().x, 0.0f);
    else 
        AddAcceleration(GetSpeed().x, 0.0f);

    if (GetState() == EntityState::Idle) 
        SetState(EntityState::Walking);
}

void Character::Jump()
{
    if (GetState() == EntityState::Dying || GetState() == EntityState::Hurt || GetState() == EntityState::Jumping)
        return;

    SetState(EntityState::Jumping);

    // Instead of jumping immediately, we buffer the input for 0.15 seconds
    m_jumpBufferTimer = 0.15f;
}

void Character::CancelJump()
{
    // VJH (Variable Jump Height)
    // Cut the upward velocity in half if the jump button is released early
    if (GetVelocity().y < 0.0f)
    {
        SetVelocity(GetVelocity().x, GetVelocity().y * 0.5f);
    }
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
            m_sprite->Load("media/spritesheets/" + sheetPath);
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
        // TODO: Togliere?
        else if (type == "JumpVelocity") keystream >> m_jumpVelocity;
        else std::cerr << "! Unknown type in character file: " << type << '\n';

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

void Character::Animate()
{
    EntityState state = GetState();

    SpriteSheet& spriteSheet = m_sprite->GetSpriteSheet();

    if (state == EntityState::Walking && spriteSheet.GetCurrentAnim()->GetName() != "Walk")
    {
        spriteSheet.SetAnimation("Walk", true, true);
    }
    else if (state == EntityState::Jumping && spriteSheet.GetCurrentAnim()->GetName() != "Jump")
    {
        spriteSheet.SetAnimation("Jump", true, false);
    }
    else if (state == EntityState::Attacking && spriteSheet.GetCurrentAnim()->GetName() != "Attack")
    {
        spriteSheet.SetAnimation("Attack", true, false);
    }
    else if (state == EntityState::Hurt && spriteSheet.GetCurrentAnim()->GetName() != "Hurt")
    {
        spriteSheet.SetAnimation("Hurt", true, false);
    }
    else if (state == EntityState::Dying && spriteSheet.GetCurrentAnim()->GetName() != "Death")
    {
        spriteSheet.SetAnimation("Death", true, false);
    }
    else if (state == EntityState::Idle && spriteSheet.GetCurrentAnim()->GetName() != "Idle")
    {
        spriteSheet.SetAnimation("Idle", true, true);
    }
}

void Character::Update(float deltaTime)
{
    // Coyote Time
    // If the character is grounded (state is not Jumping), keep the timer full
    if (GetState() != EntityState::Jumping)
    {
        m_coyoteTimer = 0.1f; // 0.1 seconds of leniency after walking off a ledge
    }
    else
    {
        // Decrease the timer while in the air
        m_coyoteTimer -= deltaTime;
    }

    // Jump Buffer
    // Decrease the jump buffer timer
    m_jumpBufferTimer -= deltaTime;

    // Jump Execution
    // If the player wants to jump (buffer active) AND can jump (coyote time active)
    if (m_jumpBufferTimer > 0.0f && m_coyoteTimer > 0.0f)
    {
        // Reset timers to prevent double jumps
        m_jumpBufferTimer = 0.0f;
        m_coyoteTimer = 0.0f;

        SetState(EntityState::Jumping);
        SetVelocity(GetVelocity().x, -m_jumpVelocity);
    }

    EntityBase::Update(deltaTime);

    if (m_attackAABB.size.x != 0.0f && m_attackAABB.size.y != 0.0f)
    {
        UpdateAttackAABB();
    }

    if (GetState() != EntityState::Dying && GetState() != EntityState::Attacking && GetState() != EntityState::Hurt)
    {
        if (std::abs(GetVelocity().y) > 0.01f || !m_collider->GetReferenceTile())
        {
            SetState(EntityState::Jumping);
        }
        else if (std::abs(GetVelocity().x) > 0.2f && !m_collider->IsCollidingX())
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
        if (!m_sprite->GetSpriteSheet().GetCurrentAnim()->IsPlaying())
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
    m_sprite->Update(deltaTime);
    m_sprite->GetSpriteSheet().SetSpritePosition(GetPosition());
}

int Character::GetHitPoints() const { return m_hitPoints; }
int Character::GetMaxHitPoints() const { return m_maxHitPoints; }