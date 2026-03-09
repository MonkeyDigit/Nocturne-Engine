#include <fstream>
#include <sstream>

#include "SpriteSheet.h"
#include "Utilities.h"
#include "EngineLog.h"
#include "ConfigParseUtils.h"

namespace
{
    using ParseUtils::TryReadExact;
    using ParseUtils::PrepareConfigLine;

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
        const float signedX = (dir == Direction::Left) ? -scale.x : scale.x;
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

bool SpriteSheet::LoadSheet(const std::string& file)
{
    // Reset previous data to avoid leaking texture references on reload
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

        if (!PrepareConfigLine(line)) continue;

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

            Animation anim;
            anim.Configure(name, startFrame, endFrame, row, frameTime, loop == 1);

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