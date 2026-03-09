#pragma once
#include <string>
struct GameplayTuning;

namespace GameplayTuningLoader
{
    void Load(const std::string& path, GameplayTuning& outTuning);
}