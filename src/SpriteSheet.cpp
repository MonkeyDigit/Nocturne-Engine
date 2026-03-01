#include "SpriteSheet.h"
#include "Utilities.h"
#include <fstream>
#include <sstream>
#include <iostream>

SpriteSheet::SpriteSheet(TextureManager& textureManager)
    : m_textureManager(textureManager),
    m_animationCurrent(nullptr),
    m_dir(Direction::Right),
    m_spriteScale(1.f, 1.f) {
}

bool SpriteSheet::LoadSheet(const std::string& file)
{
    std::ifstream sheetFile{ Utils::GetWorkingDirectory() + file };
    if (!sheetFile.is_open())
    {
        std::cerr << "! SpriteSheet could not load file: " << file << "\n";
        return false;
    }

    std::string line;
    while (std::getline(sheetFile, line))
    {
        if (line.empty() || line[0] == '|') continue;

        std::stringstream keystream(line);
        std::string type;
        keystream >> type;

        if (type == "Texture")
        {
            // TODO: Clausola duplicate entries ???

            keystream >> m_textureName;
            // Ask the TextureManager for the resource. If it works, construct the sprite
            if (m_textureManager.RequireResource(m_textureName))
            {
                m_sprite.emplace(*m_textureManager.GetResource(m_textureName));
            }
        }
        else if (type == "Size")
        {
            keystream >> m_spriteSize.x >> m_spriteSize.y;
            // Set Origin to bottom-center
            if (m_sprite) m_sprite->setOrigin({ m_spriteSize.x / 2.0f, static_cast<float>(m_spriteSize.y) });
        }
        else if (type == "Scale")
        {
            keystream >> m_spriteScale.x >> m_spriteScale.y;
            if (m_sprite) m_sprite->setScale(m_spriteScale);
        }
        else if (type == "Animation")
        {
            // TODO: Clausola duplicate e invalid ?
            Animation anim;
            keystream.clear();
            keystream.seekg(0);
            keystream >> anim;      // Use overloaded operator
            m_animations.emplace(anim.GetName(), anim);
        }
    }
    sheetFile.close();
    return true;
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

    /* TODO: Aggiungere questa clausola ?
    if (l_name == m_animationCurrent.GetName() &&
		l_play == m_animationCurrent.IsPlaying() && 
		l_loop == m_animationCurrent.IsLooped())
		return false;
    */

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