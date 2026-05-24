//
// Created by Andreas Royset on 5/20/26.
//

#ifndef PIECEDRAWER_H
#define PIECEDRAWER_H

#include <iostream>
#include <SFML/Graphics.hpp>

#include "../Board.h"

class PieceDrawer {
    sf::Texture textureMap;
    sf::Sprite pieceMarker{textureMap};
    sf::Vector2u textureSize;
    float tileSize;
    sf::Font font;
    sf::Text coordinateText{font};
    bool fontLoaded = false;

    sf::RectangleShape background;
    sf::Color white{235, 236, 208};
    sf::Color green{115, 149, 82};
    sf::Color whiteHighlighted{245, 246, 141};
    sf::Color greenHighlighted{186, 201, 73};
    sf::Color red{200, 75, 70};

public:

    explicit PieceDrawer(float tileSize) : tileSize(tileSize) {
        if (!textureMap.loadFromFile("Render/ChessPieces.png")) {
            std::cerr << "Error loading texture!" << std::endl;
            return;
        }
        textureMap.setSmooth(true);
        pieceMarker.setTexture(textureMap);
        textureSize = textureMap.getSize();

        float scaleX = tileSize/(float(textureSize.x)/6);
        float scaleY = tileSize/(float(textureSize.y)/2);

        pieceMarker.setScale({scaleX, scaleY});

        background.setSize({tileSize, tileSize});

        fontLoaded = font.openFromFile("/Users/viking/Desktop/C++ Projects/Courier Prime.ttf");
        if (!fontLoaded) {
            std::cerr << "Error loading coordinate font\n";
        }

        coordinateText.setCharacterSize(unsigned(tileSize * 0.18f));
        coordinateText.setStyle(sf::Text::Bold);
    }

    void drawPiece(sf::RenderWindow& window, Piece piece, Position position, bool highlighted, bool blackAtBottom) {

        bool lightSquare = position.rank()%2 == 0 xor position.file()%2 == 0;
        int displayFile = blackAtBottom ? 7 - position.file() : position.file();
        int displayRank = blackAtBottom ? position.rank() : 7 - position.rank();

        if (lightSquare) {
            background.setFillColor(highlighted ? whiteHighlighted : white);
        } else {
            background.setFillColor(highlighted ? greenHighlighted : green);
        }

        background.setPosition({float(displayFile)*tileSize, float(displayRank)*tileSize});
        window.draw(background);

        if (position.isNone()) return;

        if (piece != EMPTY) {

            int x, y;
            getSpriteSheetCoords(piece, x, y);

            pieceMarker.setTextureRect(sf::IntRect({int(x*textureSize.x/6), int(y*textureSize.y/2)}, {int(textureSize.x)/6, int(textureSize.y)/2}));
            pieceMarker.setPosition({tileSize*float(displayFile), tileSize*float(displayRank)});
            window.draw(pieceMarker);
        }

        drawCoordinates(window, position, lightSquare, blackAtBottom);
    }

    private:

    void drawCoordinates(sf::RenderWindow& window, Position position, bool lightSquare, bool blackAtBottom) {
        if (!fontLoaded) return;

        coordinateText.setFillColor(lightSquare ? green : white);

        int displayFile = blackAtBottom ? 7 - position.file() : position.file();
        int displayRank = blackAtBottom ? position.rank() : 7 - position.rank();
        float x = float(displayFile) * tileSize;
        float y = float(displayRank) * tileSize;
        float margin = tileSize * 0.05f;

        if (displayFile == 0) {
            coordinateText.setString(std::to_string(position.rank() + 1));
            coordinateText.setPosition({x + margin, y + margin * 0.5f});
            window.draw(coordinateText);
        }

        if (displayRank == 7) {
            coordinateText.setString(std::string(1, char('a' + position.file())));
            coordinateText.setPosition({x + tileSize - margin * 3.0f, y + tileSize - margin * 4.0f});
            window.draw(coordinateText);
        }
    }

    static void getSpriteSheetCoords(Piece piece, int&x, int& y) {
        switch (piece) {
            case BLACK_KING:
                x=0;
                y=1;
                break;
            case BLACK_QUEEN:
                x=1;
                y=1;
                break;
            case BLACK_BISHOP:
                x=2;
                y=1;
                break;
            case BLACK_KNIGHT:
                x=3;
                y=1;
                break;
            case BLACK_ROOK:
                x=4;
                y=1;
                break;
            case BLACK_PAWN:
                x=5;
                y=1;
                break;
            case WHITE_KING:
                x=0;
                y=0;
                break;
            case WHITE_QUEEN:
                x=1;
                y=0;
                break;
            case WHITE_BISHOP:
                x=2;
                y=0;
                break;
            case WHITE_KNIGHT:
                x=3;
                y=0;
                break;
            case WHITE_ROOK:
                x=4;
                y=0;
                break;
            case WHITE_PAWN:
                x=5;
                y=0;
                break;
            default:
                x=0;
                y=0;
                break;
        }
    }
};

#endif //PIECEDRAWER_H
