#pragma once
#include "Component.h"
#include "SpriteSheet.h"

class CSprite : public Component
{
public:
    // The constructor takes the TextureManager needed to initialize the SpriteSheet
    // TODO: per le texture ci va il puntatore?
    CSprite(EntityBase* owner, TextureManager& textureManager)
        : Component(owner), m_spriteSheet(textureManager) {
    }

    // Overriding the Component standard methods
    void Update(float deltaTime) override
    {
        m_spriteSheet.Update(deltaTime);
    }

    void Draw(sf::RenderWindow& window) override
    {
        m_spriteSheet.Draw(window);
    }

    // --- DELEGATED METHODS ---
    void Load(const std::string& path) { m_spriteSheet.LoadSheet(path); }
    SpriteSheet& GetSpriteSheet() { return m_spriteSheet; }

private:
    SpriteSheet m_spriteSheet;
};