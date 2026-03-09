#include "Map.h"
#include "MapTmjLoader.h"

bool Map::LoadMap(const std::string& path)
{
    return MapTmjLoader::Load(*this, path);
}