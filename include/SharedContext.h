#pragma once

// Forward declarations instead of includes (this can reduces recompilation times)
class Window;
class EventManager;
class TextureManager;
class EntityManager;
class Map;
class DebugOverlay;

struct SharedContext {
    // Constructor requires all core systems to be passed as references
    SharedContext(Window& window, EventManager& eventManager,
        TextureManager& textureManager, EntityManager& entityManager)
        : m_window(window),
        m_eventManager(eventManager),
        m_textureManager(textureManager),
        m_entityManager(entityManager),
        m_gameMap(nullptr),
        m_debugOverlay(nullptr)
    {}

    // --- CORE SYSTEMS (Always exist, cannot be null) ---
    Window& m_window;
    EventManager& m_eventManager;
    TextureManager& m_textureManager;
    EntityManager& m_entityManager;

    // --- DYNAMIC SYSTEMS (Can be null, can be swapped) ---
    Map* m_gameMap;
    DebugOverlay* m_debugOverlay;
};