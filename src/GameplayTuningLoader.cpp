#include <fstream>
#include <sstream>
#include <algorithm>
#include "GameplayTuningLoader.h"
#include "GameplayTuning.h"
#include "Utilities.h"
#include "EngineLog.h"
#include "ConfigParseUtils.h"

namespace
{
    using ParseUtils::TryReadExact;
    using ParseUtils::PrepareConfigLine;

    void WarnLine(unsigned int line, const std::string& msg)
    {
        EngineLog::Warn("gameplay.cfg line " + std::to_string(line) + ": " + msg);
    }
}

void GameplayTuningLoader::Load(const std::string& path, GameplayTuning& outTuning)
{
    std::ifstream file(Utils::GetWorkingDirectory() + path);
    if (!file.is_open())
    {
        EngineLog::WarnOnce("gameplay.config.missing", "Missing gameplay config '" + path + "'. Using defaults.");
        return;
    }

    std::string line;
    unsigned int lineNumber = 0;

    while (std::getline(file, line))
    {
        ++lineNumber;

        if (!PrepareConfigLine(line)) continue;

        std::stringstream ss(line);
        std::string key;
        if (!(ss >> key)) continue;

        if (key == "PlayerSwordKnockback")
        {
            float x = 0.0f, y = 0.0f;
            if (!TryReadExact(ss, x, y) || x < 0.0f) { WarnLine(lineNumber, "Invalid PlayerSwordKnockback"); continue; }
            outTuning.m_playerSwordKnockbackX = x;
            outTuning.m_playerSwordKnockbackY = y;
        }
        else if (key == "EnemyAttackKnockback")
        {
            float x = 0.0f, y = 0.0f;
            if (!TryReadExact(ss, x, y) || x < 0.0f) { WarnLine(lineNumber, "Invalid EnemyAttackKnockback"); continue; }
            outTuning.m_enemyAttackKnockbackX = x;
            outTuning.m_enemyAttackKnockbackY = y;
        }
        else if (key == "ProjectileFallbackSize")
        {
            float w = 0.0f, h = 0.0f;
            if (!TryReadExact(ss, w, h) || w <= 0.0f || h <= 0.0f) { WarnLine(lineNumber, "Invalid ProjectileFallbackSize"); continue; }
            outTuning.m_projectileFallbackWidth = w;
            outTuning.m_projectileFallbackHeight = h;
        }
        else if (key == "ProjectileFallbackSheet")
        {
            std::string value;
            if (!TryReadExact(ss, value) || value.empty()) { WarnLine(lineNumber, "Invalid ProjectileFallbackSheet"); continue; }
            outTuning.m_projectileFallbackSheet = value;
        }
        else if (key == "ProjectileFallbackAnimation")
        {
            std::string value;
            if (!TryReadExact(ss, value) || value.empty()) { WarnLine(lineNumber, "Invalid ProjectileFallbackAnimation"); continue; }
            outTuning.m_projectileFallbackAnimation = value;
        }
        else if (key == "CameraTargetVerticalBias")
        {
            float value = 0.0f;
            if (!TryReadExact(ss, value)) { WarnLine(lineNumber, "Invalid CameraTargetVerticalBias"); continue; }
            outTuning.m_cameraTargetVerticalBias = std::clamp(value, 0.0f, 1.0f);
        }
        else if (key == "FixedUpdateHz")
        {
            float value = 0.0f;
            if (!TryReadExact(ss, value) || value <= 0.0f) { WarnLine(lineNumber, "Invalid FixedUpdateHz"); continue; }
            outTuning.m_fixedUpdateHz = value;
        }
        else if (key == "MaxFrameTimeSeconds")
        {
            float value = 0.0f;
            if (!TryReadExact(ss, value) || value <= 0.0f) { WarnLine(lineNumber, "Invalid MaxFrameTimeSeconds"); continue; }
            outTuning.m_maxFrameTimeSeconds = value;
        }
        else if (key == "MaxUpdatesPerFrame")
        {
            int value = 0;
            if (!TryReadExact(ss, value) || value <= 0) { WarnLine(lineNumber, "Invalid MaxUpdatesPerFrame"); continue; }
            outTuning.m_maxUpdatesPerFrame = static_cast<unsigned int>(value);
        }
        else
        {
            WarnLine(lineNumber, "Unknown key '" + key + "'");
        }
    }
}