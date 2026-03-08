#pragma once
#include <cassert>

// Forward declarations instead of includes (this can reduces recompilation times)
class Window;
class EventManager;
class TextureManager;
class EntityManager;
class AudioManager;
class Map;

struct SharedContext {
    // Constructor requires all core systems to be passed as references
    SharedContext(Window& window, EventManager& eventManager,
        TextureManager& textureManager, AudioManager& audioManager)
        : m_window(window),
        m_textureManager(textureManager),
        m_eventManager(eventManager),
        m_entityManager(nullptr),
        m_audioManager(audioManager),
        m_gameMap(nullptr)
    {}
    // EntityManager is injected after construction (breaks circular init dependency)
    void SetEntityManager(EntityManager& entityManager) { m_entityManager = &entityManager; }
    bool HasEntityManager() const { return m_entityManager != nullptr; }

    EntityManager& GetEntityManager() const
    {
        // Contract check: EntityManager must be injected before use
        assert(m_entityManager && "SharedContext: EntityManager is not initialized");
        return *m_entityManager;
    }
    // --- CORE SYSTEMS (Always exist, cannot be null) ---
    Window& m_window;
    TextureManager& m_textureManager;
    EventManager& m_eventManager;
    AudioManager& m_audioManager;

    // --- DYNAMIC SYSTEMS (Can be null, can be swapped) ---
    EntityManager* m_entityManager;
    Map* m_gameMap;
};