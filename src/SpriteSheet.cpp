#include <cmath>
#include <fstream>
#include <sstream>
#include "SpriteSheet.h"
#include "Utilities.h"
#include "EngineLog.h"

namespace
{
    template <typename... Args>
    bool TryReadExact(std::stringstream& stream, Args&... args)
    {
        if (!((stream >> args) && ...))
            return false;

        std::string trailing;
        return !(stream >> trailing); // Reject extra tokens
    }

    void WarnSheetValue(
        const std::string& file,
        unsigned int lineNumber,
        const std::string& key,
        const std::string& expected)
    {
        EngineLog::Warn(
            "Invalid '" + key + "' in sheet '" + file + "' at line " +
            std::to_string(lineNumber) + " (expected: " + expected + ")");
    }

    void ApplySpriteOrigin(sf::Sprite& sprite, const sf::Vector2i& size)
    {
        if (size.x <= 0 || size.y <= 0) return;
        // Character pivot is bottom-center for better platformer alignment
        sprite.setOrigin({ size.x / 2.0f, static_cast<float>(size.y) });
    }

    void ApplySpriteScale(sf::Sprite& sprite, const sf::Vector2f& scale, Direction dir)
    {
        const float absX = std::abs(scale.x);
        const float signedX = (dir == Direction::Left) ? -absX : absX;
        sprite.setScale({ signedX, scale.y });
    }

    bool TryParseAnimationLine(
        std::stringstream& stream,
        std::string& outName,
        unsigned int& outStartFrame,
        unsigned int& outEndFrame,
        unsigned int& outRow,
        float& outFrameTime,
        int& outLoop)
    {
        if (!TryReadExact(
            stream, outName, outStartFrame, outEndFrame, outRow, outFrameTime, outLoop))
        {
            return false;
        }

        if (outName.empty()) return false;
        if (outEndFrame < outStartFrame) return false;
        if (outFrameTime <= 0.0f) return false;
        if (outLoop != 0 && outLoop != 1) return false;

        return true;
    }
}

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
    // Reset previous data to avoid leaking texture references on reload.
    ReleaseCurrentTexture();
    m_animations.clear();
    m_animationCurrent = nullptr;
    m_spriteSize = { 0, 0 };
    m_spriteScale = { 1.0f, 1.0f };

    bool hasTexture = false;
    bool hasSize = false;

    std::ifstream sheetFile{ Utils::GetWorkingDirectory() + file };
    if (!sheetFile.is_open())
    {
        EngineLog::Error("SpriteSheet could not load file: " + file);
        return false;
    }

    std::string line;
    unsigned int lineNumber = 0;

    while (std::getline(sheetFile, line))
    {
        ++lineNumber;

        // Support inline comments and full-line comments
        const size_t commentPos = line.find('#');
        if (commentPos != std::string::npos)
            line.erase(commentPos);

        const size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) continue; // empty/whitespace
        if (line[first] == '|') continue;         // custom comment marker

        std::stringstream keystream(line);
        std::string type;
        if (!(keystream >> type)) continue;

        if (type == "Texture")
        {
            std::string textureName;
            if (!TryReadExact(keystream, textureName) || textureName.empty())
            {
                WarnSheetValue(file, lineNumber, type, "Texture <textureId>");
                continue;
            }

            // If Texture appears more than once, last valid one wins
            if (!m_textureName.empty())
            {
                m_textureManager.ReleaseResource(m_textureName);
                m_textureName.clear();
                m_sprite.reset();
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

            // Re-apply cached size/scale when texture gets created
            ApplySpriteOrigin(*m_sprite, m_spriteSize);
            ApplySpriteScale(*m_sprite, m_spriteScale, m_dir);

            hasTexture = true;
        }
        else if (type == "Size")
        {
            int width = 0;
            int height = 0;
            if (!TryReadExact(keystream, width, height) || width <= 0 || height <= 0)
            {
                WarnSheetValue(file, lineNumber, type, "Size <width > 0> <height > 0>");
                continue;
            }

            m_spriteSize = { width, height };
            hasSize = true;

            if (m_sprite)
                ApplySpriteOrigin(*m_sprite, m_spriteSize);
        }
        else if (type == "Scale")
        {
            float scaleX = 0.0f;
            float scaleY = 0.0f;
            if (!TryReadExact(keystream, scaleX, scaleY) || scaleX <= 0.0f || scaleY <= 0.0f)
            {
                WarnSheetValue(file, lineNumber, type, "Scale <x > 0> <y > 0>");
                continue;
            }

            m_spriteScale = { scaleX, scaleY };

            if (m_sprite)
                ApplySpriteScale(*m_sprite, m_spriteScale, m_dir);
        }
        else if (type == "Animation")
        {
            std::string name;
            unsigned int startFrame = 0;
            unsigned int endFrame = 0;
            unsigned int row = 0;
            float frameTime = 0.0f;
            int loop = 0;

            if (!TryParseAnimationLine(
                keystream, name, startFrame, endFrame, row, frameTime, loop))
            {
                WarnSheetValue(
                    file,
                    lineNumber,
                    type,
                    "Animation <name> <startFrame> <endFrame>=start.. <row> <frameTime > 0> <loop 0|1>");
                continue;
            }

            // Reuse Animation stream parser to keep one source of truth
            std::stringstream animationStream;
            animationStream << "Animation "
                << name << ' '
                << startFrame << ' '
                << endFrame << ' '
                << row << ' '
                << frameTime << ' '
                << loop;

            Animation anim;
            if (!(animationStream >> anim))
            {
                EngineLog::Warn(
                    "Failed to parse animation in sheet '" + file +
                    "' at line " + std::to_string(lineNumber));
                continue;
            }

            const bool existed = (m_animations.find(name) != m_animations.end());
            m_animations.insert_or_assign(name, anim);

            if (existed)
            {
                EngineLog::WarnOnce(
                    "sheet.duplicate_anim." + file + "." + name,
                    "Duplicate animation '" + name + "' in sheet '" + file + "'. Last definition wins.");
            }
        }
        else
        {
            EngineLog::Warn(
                "Unknown key '" + type + "' in sheet '" + file +
                "' at line " + std::to_string(lineNumber));
        }
    }

    if (!hasTexture || !m_sprite)
    {
        EngineLog::Error("Sheet loaded without a valid Texture section: " + file);
        return false;
    }

    if (!hasSize || m_spriteSize.x <= 0 || m_spriteSize.y <= 0)
    {
        EngineLog::Error("Sheet loaded without a valid Size section: " + file);
        return false;
    }

    if (m_animations.empty())
    {
        EngineLog::Error("Sheet '" + file + "' has no valid Animation entries.");
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