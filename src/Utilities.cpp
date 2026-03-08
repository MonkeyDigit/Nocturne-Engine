#include <SFML/Graphics/Font.hpp>
#include "Utilities.h"
#include "EngineLog.h"

std::string Utils::GetWorkingDirectory()
{
    return fs::current_path().string() + "\\";
}

bool Utils::LoadFontOrWarn(
    sf::Font& font,
    const std::string& path,
    const std::string& ownerTag,
    const std::string& fontTag)
{
    if (path.empty())
    {
        EngineLog::WarnOnce(
            "font.empty_path." + ownerTag + "." + fontTag,
            ownerTag + ": empty font path for '" + fontTag + "'");
        return false;
    }

    if (font.openFromFile(path))
        return true;

    EngineLog::WarnOnce(
        "font.load_failed." + ownerTag + "." + fontTag,
        ownerTag + ": failed to load font '" + fontTag + "' from '" + path + "'");
    return false;
}