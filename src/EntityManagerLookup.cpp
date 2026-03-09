#include "EntityManager.h"
#include "EngineLog.h"

EntityBase* EntityManager::GetPlayer()
{
    if (m_playerId >= 0)
    {
        if (EntityBase* cached = Find(static_cast<unsigned int>(m_playerId)))
            return cached;

        // Cached id is stale
        m_playerId = -1;
    }

    EntityBase* foundPlayer = nullptr;
    unsigned int foundId = 0u;

    for (auto& [id, entity] : m_entities)
    {
        if (!entity || entity->GetType() != EntityType::Player)
            continue;

        if (!foundPlayer)
        {
            foundPlayer = entity.get();
            foundId = id;
        }
        else
        {
            EngineLog::WarnOnce(
                "entity.player.multiple",
                "Multiple Player entities found. Using the first one.");
            break;
        }
    }

    if (foundPlayer)
    {
        m_playerId = static_cast<int>(foundId);
        return foundPlayer;
    }

    m_playerId = -1;
    return nullptr;
}