#pragma once
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <string>
#include <optional>

class StateManager; // Forward declaration

class BaseState
{
public:
    // Takes StateManager by reference (guaranteed to exist)
    BaseState(StateManager& stateManager);
    virtual ~BaseState() = default;

    virtual void OnCreate() = 0;
    virtual void OnDestroy() = 0;

    virtual void Activate() = 0;
    virtual void Deactivate() = 0;

    virtual void Update(const sf::Time& time) = 0;
    virtual void Draw() = 0;

    void SetTransparent(bool transparent);
    void SetTranscendent(bool transcendence);

    bool IsTransparent() const;
    bool IsTranscendent() const;

    // Returning references
    StateManager& GetStateManager();


protected:
    StateManager& m_stateManager;
    bool m_transparent;
    bool m_transcendent;

    // Shared texture tracking for UI/menu states
    sf::Texture* AcquireTrackedTexture(const std::string& id, const std::string& ownerTag);
    void ReleaseTrackedTextures(const std::string& ownerTag);
    bool SetupCenteredBackground(
        std::optional<sf::Sprite>& outSprite,
        const std::string& textureId,
        const sf::Vector2f& uiResolution,
        const std::string& ownerTag,
        bool fitToHeight = false);

    void CenterText(sf::Text& text, float x, float y);

    std::unordered_map<std::string, unsigned int> m_trackedTextureRefs;

    // Shared helper for consistent font loading diagnostics
    bool LoadFontOrWarn(
        sf::Font& font,
        const std::string& path,
        const std::string& ownerTag,
        const std::string& fontTag);
};