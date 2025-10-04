#pragma once
#include <SFML/Graphics.hpp>

class ICpuTLPView {
public:
    virtual ~ICpuTLPView() = default;
    virtual void handleEvent(sf::Event&) {}
    virtual void update(float) {}
    // Se asume que el contenedor (State) ya abri� un BeginChild para el �rea derecha
    virtual void render() = 0;
};
