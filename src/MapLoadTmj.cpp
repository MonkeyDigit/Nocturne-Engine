#include "Map.h"
#include "MapTmjLoader.h"

void Map::LoadMap(const std::string& path)
{
    MapTmjLoader::Load(*this, path);
}