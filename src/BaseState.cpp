#include "BaseState.h"
#include "StateManager.h"
#include "SharedContext.h"
#include "Window.h"

BaseState::BaseState(StateManager& stateManager)
    : m_stateManager(stateManager),
    m_transparent(false),
    m_transcendent(false)
{
}

void BaseState::SetTransparent(bool transparent) { m_transparent = transparent; }
void BaseState::SetTranscendent(bool transcendence) { m_transcendent = transcendence; }
void BaseState::SetView(const sf::View& view) { m_view = view; }

bool BaseState::IsTransparent() const { return m_transparent; }
bool BaseState::IsTranscendent() const { return m_transcendent; }

StateManager& BaseState::GetStateManager() { return m_stateManager; }
sf::View& BaseState::GetView() { return m_view; }

void BaseState::AdjustView()
{
    AdjustView(m_view);
}

void BaseState::AdjustView(sf::View& view)
{
    // Compares the aspect ratio of the window to the aspect ratio of the view,
    // and sets the view's viewport accordingly in order to archieve a letterbox effect.
    // A new view (with a new viewport set) is returned.
    sf::Vector2u windowSize = m_stateManager.GetContext().m_window.GetWindowSize();

    float windowRatio = windowSize.x / static_cast<float>(windowSize.y);
    float viewRatio = view.getSize().x / view.getSize().y;
    float sizeX = 1.0f;
    float sizeY = 1.0f;
    float posX = 0.0f;
    float posY = 0.0f;

    bool horizontalSpacing = true;
    if (windowRatio < viewRatio)
        horizontalSpacing = false;

    // If horizontalSpacing is true, the black bars will appear on the left and right side.
    // Otherwise, the black bars will appear on the top and bottom.


    if (horizontalSpacing)
    {
        sizeX = viewRatio / windowRatio;
        posX = (1.0f - sizeX) / 2.0f;
    }
    else
    {
        sizeY = windowRatio / viewRatio;
        posY = (1.0f - sizeY) / 2.0f;
    }

    // SFML 3: FloatRect requires explicit vectors for position and size
    view.setViewport(sf::FloatRect({ posX, posY }, { sizeX, sizeY }));
}