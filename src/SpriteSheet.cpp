#include <fstream>
#include <sstream>
#include "SpriteSheet.h"
#include "Utilities.h"
#include "EngineLog.h"

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

bool SpriteSheet::LoadSheet(const std::string& file)
{
    // Reset previous data to avoid leaking texture references on reload
    ReleaseCurrentTexture();
    m_animations.clear();
    m_animationCurrent = nullptr;

    std::ifstream sheetFile{ Utils::GetWorkingDirectory() + file };
    if (!sheetFile.is_open())
    {
        EngineLog::Error("SpriteSheet could not load file: " + file);
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
            std::string textureName;
            keystream >> textureName;

            if (textureName.empty())
            {
                EngineLog::Error("Missing texture id in sheet: " + file);
                return false;
            }

            if (!m_textureManager.RequireResource(textureName))
            {
                EngineLog::Error("Failed to require texture: " + textureName);
                return false;
            }

            sf::Texture* texture = m_textureManager.GetResource(textureName);
            if (!texture)
            {
                EngineLog::Error("Texture not found after require: " + textureName);
                return false;
            }

            m_textureName = textureName;
            m_sprite.emplace(*texture);
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
            Animation anim;
            keystream.clear();
            keystream.seekg(0);

            if (!(keystream >> anim))
            {
                EngineLog::Warn("Invalid Animation entry in sheet: " + file);
                continue;
            }

            const std::string animName = anim.GetName();
            if (animName.empty())
            {
                EngineLog::Warn("Animation with empty name in sheet: " + file);
                continue;
            }

            const bool existed = (m_animations.find(animName) != m_animations.end());
            m_animations.insert_or_assign(animName, anim);

            if (existed)
            {
                EngineLog::WarnOnce(
                    "sheet.duplicate_anim." + file + "." + animName,
                    "Duplicate animation '" + animName + "' in sheet '" + file + "'. Last definition wins.");
            }
        }
    }
    sheetFile.close();

    if (!m_sprite)
    {
        EngineLog::Error("Sheet loaded without a valid texture section: " + file);
        return false;
    }

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