#include <fstream>
#include <sstream>

#include "EntityManager.h"
#include "Utilities.h"
#include "EngineLog.h"
#include "ConfigParseUtils.h"

void EntityManager::LoadEnemyTypes(const std::string& path)
{
    std::ifstream file{ Utils::GetWorkingDirectory() + path };
    if (!file.is_open())
    {
        EngineLog::Error("Failed loading enemy type file: " + path);
        return;
    }

    auto warnLine = [&](unsigned int lineNumber, const std::string& message)
        {
            EngineLog::Warn(path + " line " + std::to_string(lineNumber) + ": " + message);
        };

    std::string line;
    unsigned int lineNumber = 0;

    while (std::getline(file, line))
    {
        ++lineNumber;

        if (!ParseUtils::PrepareConfigLine(line)) continue;

        std::stringstream keystream(line);
        std::string enemyName;
        std::string charFile;

        if (!(keystream >> enemyName >> charFile))
        {
            warnLine(lineNumber, "Invalid entry (expected: <EnemyTypeId> <CharacterFile>).");
            continue;
        }

        std::string trailing;
        if (keystream >> trailing)
        {
            warnLine(lineNumber, "Too many tokens for enemy type '" + enemyName + "'.");
            continue;
        }

        auto [it, inserted] = m_enemyTypes.emplace(enemyName, charFile);
        if (!inserted)
        {
            warnLine(lineNumber, "Duplicate enemy type '" + enemyName + "'. Overwriting previous file.");
            it->second = charFile;
        }
    }
}