#include "core/State.h"
#include "core/StateManager.h"

State::State(StateManager* stateManager, sf::RenderWindow* window)
    : m_stateManager(stateManager)
    , m_window(window)
{
}