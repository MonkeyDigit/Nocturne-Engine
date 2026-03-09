#pragma once
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <string>
#include <optional>
#include "TextureManager.h"
#include "Animation.h"
#include "Direction.h"

class SpriteSheet
{
public:
    // Takes the TextureManager by reference, guaranteeing it exists
    SpriteSheet(TextureManager& textureManager);
    ~SpriteSheet();

    void SetSpritePosition(const sf::Vector2f& pos);
    void SetDirection(Direction dir);
    // Returns true if the animation exists and is now active
    // Returns false only when the animation name does not exist in the sheet
    bool SetAnimation(const std::string& name, bool play = true, bool loop = false);

    bool HasAnimation(const std::string& name) const;
    bool IsCurrentAnimation(const std::string& name) const;

    // Loads and parses the .sheet file
    bool LoadSheet(const std::string& file);

    Animation* GetCurrentAnim();
    sf::Vector2i GetSpriteSize() const;
    Direction GetDirection() const;
    sf::Vector2f GetPosition() const;

    void Update(float deltaTime);
    void Draw(sf::RenderWindow& window);

private:
    void CropSprite();
    void ReleaseCurrentTexture();

    std::string m_textureName;

    // We use std::optional because SFML 3 requires a texture on Sprite creation
    std::optional<sf::Sprite> m_sprite;

    sf::Vector2i m_spriteSize;
    sf::Vector2f m_spriteScale;

    std::unordered_map<std::string, Animation> m_animations;
    Animation* m_animationCurrent;
    Direction m_dir;

    TextureManager& m_textureManager;
};