#pragma once

#include "State.h"
#include <memory> // para std::unique_ptr

class StateManager {
public:
    StateManager();
    ~StateManager();

    // Cambia al estado que le pasemos INMEDIATAMENTE
    void setCurrentState(std::unique_ptr<State> newState);

    // En lugar de cambiar inmediatamente, dejamos el estado “en cola”
    void queueNextState(std::unique_ptr<State> newState);

    // Devuelve el estado actual
    State* getCurrentState() const;

    // Retorna true si hay un estado en cola
    bool hasNextState() const;

    // Si hay un estado en cola, lo aplica (y lo saca de la cola)
    void applyNextState();

private:
    std::unique_ptr<State> m_currentState;
    std::unique_ptr<State> m_nextState; // cola para el próximo estado
};