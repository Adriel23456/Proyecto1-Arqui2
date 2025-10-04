#include "StateManager.h"

StateManager::StateManager()
    : m_currentState(nullptr)
    , m_nextState(nullptr)
{
}

StateManager::~StateManager() {
    // El unique_ptr se encargará de eliminar el estado en el destructor.
}

void StateManager::setCurrentState(std::unique_ptr<State> newState) {
    // Reemplaza el estado actual por el nuevo inmediatamente
    m_currentState = std::move(newState);
}

void StateManager::queueNextState(std::unique_ptr<State> newState) {
    // Guardamos el siguiente estado en cola, sin cambiar aún
    m_nextState = std::move(newState);
}

bool StateManager::hasNextState() const {
    return (bool)m_nextState;
}

void StateManager::applyNextState() {
    if (m_nextState) {
        // Ahora sí cambiamos de estado
        setCurrentState(std::move(m_nextState));
        // m_nextState ya queda en nullptr
    }
}

State* StateManager::getCurrentState() const {
    return m_currentState.get();
}