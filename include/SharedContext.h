#pragma once

// Forward declarations instead of includes (this can reduces recompilation times)
class Window;
class EventManager;
class TextureManager;
class EntityManager;
class AudioManager;
class Map;
class DebugOverlay;

struct SharedContext {
    // Constructor requires all core systems to be passed as references
    SharedContext(Window& window, EventManager& eventManager,
        TextureManager& textureManager, EntityManager& entityManager, AudioManager& audioManager)
        : m_window(window),
        m_eventManager(eventManager),
        m_textureManager(textureManager),
        m_entityManager(entityManager),
        m_audioManager(audioManager),
        m_gameMap(nullptr),
        m_debugOverlay(nullptr)
    {}

    // --- CORE SYSTEMS (Always exist, cannot be null) ---
    Window& m_window;
    TextureManager& m_textureManager;
    EventManager& m_eventManager;
    EntityManager& m_entityManager;
    AudioManager& m_audioManager;

    // --- DYNAMIC SYSTEMS (Can be null, can be swapped) ---
    Map* m_gameMap;
    DebugOverlay* m_debugOverlay;
};