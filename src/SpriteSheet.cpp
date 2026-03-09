#include "SpriteSheet.h"

SpriteSheet::SpriteSheet(TextureManager& textureManager)
    : m_textureManager(textureManager),
    m_animationCurrent(nullptr),
    m_dir(Direction::Right),
    m_spriteScale(1.f, 1.f) {
}

SpriteSheet::~SpriteSheet()
{
    ReleaseCurrentTexture();
}

void SpriteSheet::ReleaseCurrentTexture()
{
    if (!m_textureName.empty())
    {
        m_textureManager.ReleaseResource(m_textureName);
        m_textureName.clear();
    }

    m_sprite.reset();
}

void SpriteSheet::CropSprite()
{
    if (!m_sprite || !m_animationCurrent) return;

    // SFML 3 FloatRect/IntRect syntax requires position and size vectors
    sf::IntRect rect(
        { static_cast<int>(m_animationCurrent->GetCurrentFrame() * m_spriteSize.x),
         static_cast<int>(m_animationCurrent->GetRow() * m_spriteSize.y) },
        { m_spriteSize.x, m_spriteSize.y }
    );

    m_sprite->setTextureRect(rect);
}

void SpriteSheet::SetSpritePosition(const sf::Vector2f& pos)
{
    if (m_sprite) m_sprite->setPosition(pos);
}

void SpriteSheet::SetDirection(Direction dir)
{
    if (!m_sprite || m_dir == dir) return;

    m_dir = dir;

    // Invert the X scale to flip the character horizontally
    if (m_dir == Direction::Left)
        m_sprite->setScale({ -m_spriteScale.x, m_spriteScale.y });
    else
        m_sprite->setScale({ m_spriteScale.x, m_spriteScale.y });
}

bool SpriteSheet::SetAnimation(const std::string& name, bool play, bool loop)
{
    // Do nothing if it's already playing this animation
    if (m_animationCurrent && m_animationCurrent->GetName() == name) return false;

    auto it = m_animations.find(name);

    if (it != m_animations.end())
    {
        m_animationCurrent = &it->second;
        m_animationCurrent->SetLooping(loop);
        m_animationCurrent->Stop();
        if (play) m_animationCurrent->Play();
        CropSprite();
        return true;
    }

    return false;
}

void SpriteSheet::Update(float deltaTime)
{
    if (m_animationCurrent && m_animationCurrent->IsPlaying())
    {
        m_animationCurrent->Update(deltaTime);
        CropSprite();
    }
}

void SpriteSheet::Draw(sf::RenderWindow& window)
{
    if (m_sprite) window.draw(*m_sprite);
}

Animation* SpriteSheet::GetCurrentAnim() { return m_animationCurrent; }
sf::Vector2i SpriteSheet::GetSpriteSize() const { return m_spriteSize; }
Direction SpriteSheet::GetDirection() const { return m_dir; }
sf::Vector2f SpriteSheet::GetPosition() const { return m_sprite ? m_sprite->getPosition() : sf::Vector2f(); }