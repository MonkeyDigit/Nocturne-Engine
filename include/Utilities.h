#pragma once
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace Utils
{
    std::string GetWorkingDirectory();
}