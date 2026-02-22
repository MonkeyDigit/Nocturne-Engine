#include "Utilities.h"

std::string Utils::GetWorkingDirectory()
{
	return fs::current_path().string() + "\\";
}