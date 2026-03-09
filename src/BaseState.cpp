#include "BaseState.h"
#include "StateManager.h"
#include "SharedContext.h"
#include "TextureManager.h"
#include "EngineLog.h"
#include "Utilities.h"

BaseState::BaseState(StateManager& stateManager)
    : m_stateManager(stateManager),
    m_transparent(false),
    m_transcendent(false)
{
}

void BaseState::SetTransparent(bool transparent) { m_transparent = transparent; }
void BaseState::SetTranscendent(bool transcendence) { m_transcendent = transcendence; }

bool BaseState::IsTransparent() const { return m_transparent; }
bool BaseState::IsTranscendent() const { return m_transcendent; }

StateManager& BaseState::GetStateManager() { return m_stateManager; }

bool BaseState::SetupCenteredBackground(
    std::optional<sf::Sprite>& outSprite,
    const std::string& textureId,
    const sf::Vector2f& uiResolution,
    const std::string& ownerTag,
    bool fitToHeight)
{
    sf::Texture* texture = AcquireTrackedTexture(textureId, ownerTag);
    if (!texture)
    {
        outSprite.reset();
        return false;
    }

    outSprite.emplace(*texture);

    if (fitToHeight)
    {
        const sf::Vector2u texSize = texture->getSize();
        if (texSize.y > 0)
        {
            const float scale = uiResolution.y / static_cast<float>(texSize.y);
            outSprite->setScale({ scale, scale });
        }
        else
        {
            EngineLog::WarnOnce(
                "state.background.zero_height." + ownerTag + "." + textureId,
                ownerTag + ": texture '" + textureId + "' has zero height.");
        }
    }

    outSprite->setOrigin({
        outSprite->getLocalBounds().size.x * 0.5f,
        outSprite->getLocalBounds().size.y * 0.5f
        });
    outSprite->setPosition(uiResolution * 0.5f);

    return true;
}

void BaseState::CenterText(sf::Text& text, float x, float y)
{
    const sf::FloatRect rect = text.getLocalBounds();
    text.setOrigin({
        rect.position.x + rect.size.x * 0.5f,
        rect.position.y + rect.size.y * 0.5f
        });
    text.setPosition({ x, y });
}

sf::Texture* BaseState::AcquireTrackedTexture(const std::string& id, const std::string& ownerTag)
{
    if (id.empty())
    {
        EngineLog::WarnOnce(
            "state.texture.empty_id." + ownerTag,
            ownerTag + ": attempted to acquire an empty texture id.");
        return nullptr;
    }

    SharedContext& context = m_stateManager.GetContext();
    if (!context.m_textureManager.RequireResource(id))
    {
        EngineLog::WarnOnce(
            "state.texture.require_failed." + ownerTag + "." + id,
            ownerTag + ": failed to require texture '" + id + "'.");
        return nullptr;
    }

    ++m_trackedTextureRefs[id];

    sf::Texture* texture = context.m_textureManager.GetResource(id);
    if (!texture)
    {
        EngineLog::WarnOnce(
            "state.texture.null_after_require." + ownerTag + "." + id,
            ownerTag + ": texture '" + id + "' is null after require.");
    }

    return texture;
}

void BaseState::ReleaseTrackedTextures(const std::string& ownerTag)
{
    SharedContext& context = m_stateManager.GetContext();

    for (auto& [id, count] : m_trackedTextureRefs)
    {
        for (unsigned int i = 0; i < count; ++i)
        {
            if (!context.m_textureManager.ReleaseResource(id))
            {
                EngineLog::WarnOnce(
                    "state.texture.release_failed." + ownerTag + "." + id,
                    ownerTag + ": failed to release texture '" + id + "'.");
                break;
            }
        }
    }

    m_trackedTextureRefs.clear();
}

bool BaseState::LoadFontOrWarn(
    sf::Font& font,
    const std::string& path,
    const std::string& ownerTag,
    const std::string& fontTag)
{
    return Utils::LoadFontOrWarn(font, path, ownerTag, fontTag);
}

void BaseState::SetupTextButton(
    sf::RectangleShape& rect,
    sf::Text& label,
    const sf::Vector2f& size,
    const sf::Vector2f& centerPosition,
    const std::string& text,
    unsigned int characterSize,
    const sf::Color& fillColor,
    const sf::Color& outlineColor,
    float outlineThickness)
{
    // Shared menu/settings button setup to keep style and layout consistent.
    rect.setSize(size);
    rect.setFillColor(fillColor);
    rect.setOutlineColor(outlineColor);
    rect.setOutlineThickness(outlineThickness);
    rect.setOrigin({ size.x * 0.5f, size.y * 0.5f });
    rect.setPosition(centerPosition);

    label.setString(text);
    label.setCharacterSize(characterSize);
    CenterText(label, centerPosition.x, centerPosition.y);
}

void BaseState::UpdateButtonHoverColor(
    sf::RectangleShape& rect,
    const sf::Vector2f& mousePosition,
    const sf::Color& idleColor,
    const sf::Color& hoverColor)
{
    rect.setFillColor(
        rect.getGlobalBounds().contains(mousePosition) ? hoverColor : idleColor);
}