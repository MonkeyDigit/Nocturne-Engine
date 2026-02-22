#pragma once
#include <SFML/Graphics.hpp>

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
    void SetView(const sf::View& view);

    bool IsTransparent() const;
    bool IsTranscendent() const;

    // Returning references
    StateManager& GetStateManager();
    sf::View& GetView();

    // Adjusts the view to preserve aspect ratio (letterboxing)
    void AdjustView();
    void AdjustView(sf::View& view);

protected:
    StateManager& m_stateManager;
    bool m_transparent;
    bool m_transcendent;
    sf::View m_view;
};