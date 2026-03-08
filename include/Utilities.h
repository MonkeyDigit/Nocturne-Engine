#pragma once
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace sf
{
    class Font;
}

namespace Utils
{
    std::string GetWorkingDirectory();

    // Shared font loader with consistent diagnostics
    bool LoadFontOrWarn(
        sf::Font& font,
        const std::string& path,
        const std::string& ownerTag,
        const std::string& fontTag);
}