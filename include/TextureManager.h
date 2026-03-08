#pragma once
#include <SFML/Graphics.hpp>
#include "ResourceManager.h"
#include <memory>
#include "EngineLog.h"

class TextureManager : public ResourceManager<TextureManager, sf::Texture>
{
public:
    TextureManager() : ResourceManager{ "config/textures.cfg" } {}

    std::unique_ptr<sf::Texture> Load(const std::string& path)
    {
        auto texture = std::make_unique<sf::Texture>();
        if (!texture->loadFromFile(Utils::GetWorkingDirectory() + path))
        {
            EngineLog::Error("Failed to load texture: " + path);
            return nullptr; // !! The object gets destroyed automatically when exiting out of scope
        }
        return texture;
    }
};