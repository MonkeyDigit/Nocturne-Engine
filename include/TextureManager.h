#pragma once
#include <SFML/Graphics.hpp>
#include "ResourceManager.h"
#include <memory>

class TextureManager : public ResourceManager<TextureManager, sf::Texture>
{
public:
    TextureManager() : ResourceManager{ "config/textures.cfg" } {}

    std::unique_ptr<sf::Texture> Load(const std::string& path)
    {
        auto texture = std::make_unique<sf::Texture>();
        if (!texture->loadFromFile(Utils::GetWorkingDirectory() + path))
        {
            std::cerr << "! Failed to load texture: " << path << '\n';
            return nullptr; // !! The object gets destroyed automatically when exiting out of scope
        }
        return texture;
    }
};