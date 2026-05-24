//
// Created by Andreas Royset on 5/20/26.
//

#ifndef MOVEDRAWER_H
#define MOVEDRAWER_H

#include <iostream>
#include <SFML/Graphics.hpp>

#include "../Board.h"

class MoveDrawer {
    sf::CircleShape moveMarker;
    float tileSize;

public:

    explicit MoveDrawer(float tileSize) : tileSize(tileSize) {
        moveMarker = sf::CircleShape(0.0001f, 64);
        moveMarker.setFillColor(sf::Color::Transparent);
        moveMarker.setOutlineColor(sf::Color(0, 0, 0, 100));
        moveMarker.setOutlineThickness(10);
    }

    void drawMove(sf::RenderWindow& window, const Move &move) {
        if (move.isCapture()) {
            moveMarker.setRadius(tileSize/2.5f);
            moveMarker.setOutlineThickness(10);
            moveMarker.setPosition({float(move.target().file())*tileSize+10, float(7-move.target().rank())*tileSize+10});
            window.draw(moveMarker);
        } else {
            moveMarker.setRadius(0.00001);
            moveMarker.setOutlineThickness(18);
            moveMarker.setPosition({float(move.target().file()+0.5)*tileSize, float(7-move.target().rank()+0.5)*tileSize});
            window.draw(moveMarker);
        }
    }
};

#endif //MOVEDRAWER_H
