#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include "BaseState.h"

struct SharedContext; // Forward declared

enum class StateType
{
    Intro = 1,
    MainMenu,
    Game,
    Paused,
    Settings,
    Credits
};

// Aliases
// Modern C++: unique_ptr handles state lifetimes automatically
using StateContainer = std::vector<std::pair<StateType, std::unique_ptr<BaseState>>>;
using TypeContainer = std::vector<StateType>;
using StateFactory = std::unordered_map<StateType, std::function<std::unique_ptr<BaseState>()>>;

class StateManager
{
public:
    StateManager(SharedContext& shared);
    ~StateManager(); // Custom destructor needed to call OnDestroy() on states

    void Update(const sf::Time& time);
    void Draw();
    void ProcessRequests();

    SharedContext& GetContext();
    bool HasState(StateType type) const;

    void SwitchTo(StateType type);
    void Remove(StateType type);

private:
    void CreateState(StateType type);
    void RemoveState(StateType type);

    template<class T>
    void RegisterState(StateType type)
    {
        // Lambda captures 'this' and returns a unique_ptr of the new state
        m_stateFactory[type] = [this]() -> std::unique_ptr<BaseState>       // -> std::unique_ptr<BaseState> indicates the return type of the lambda expression
            {
                return std::make_unique<T>(*this);
            };
    }

    SharedContext& m_shared;
    StateContainer m_states;
    TypeContainer m_toRemove;
    StateFactory m_stateFactory;
    sf::Vector2u m_windSize;
};