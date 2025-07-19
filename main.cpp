#include <iostream>
#include "Board.h"
#include <SFML/Graphics.hpp>

int main() {
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "Fullscreen Window", sf::Style::Fullscreen, settings);
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile("/System/Library/Fonts/Helvetica.ttc")) {
        std::cerr << "Error loading font\n";
        return -1;
    }

    sf::Text text;
    text.setFont(font);             // Set the font
    text.setCharacterSize(12);       // Set the font size (in pixels)
    text.setFillColor(sf::Color::White); // Set the text color
    text.setPosition(900, 0);

    sf::RectangleShape background = sf::RectangleShape(sf::Vector2f(112.5f, 112.5f));
    sf::Color green = sf::Color(120, 155, 100);
    sf::Color white = sf::Color(217, 213, 186);
    sf::Color yellow = sf::Color(244,  247, 148);
    sf::Color red = sf::Color(200, 75, 70);

    sf::CircleShape moveMarker = sf::CircleShape(0.0001);
    moveMarker.setFillColor(sf::Color::Transparent);
    moveMarker.setOutlineColor(sf::Color(0, 0, 0, 100));
    moveMarker.setOutlineThickness(10);

    sf::Texture textureMap;
    if (!textureMap.loadFromFile("/Users/viking/desktop/C++/Chess/ChessPieces.png")) {
        std::cerr << "Error loading texture!" << std::endl;
        return -1;
    }
    auto* pieceMarker = new sf::Sprite();
    sf::Vector2 textureSize = sf::Vector2f(389, 129);
    pieceMarker->setScale(float(112.5f/(textureSize.x/6)*0.8), float(112.5f/(textureSize.y/2)*0.8));

    int startRank = 8;
    int startFile = 8;

    Board board = Board();

    BoardState* validMoves = board.board;

    sf::Clock clock;
    while (window.isOpen()) {
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        float deltaTime = clock.restart().asSeconds();
        if (board.KingInCheck(board.board, false)) {
            text.setString(" Fps: " + std::to_string(1/deltaTime) + "Check");
        } else {
            text.setString(" Fps: " + std::to_string(1/deltaTime));
        }


        if (event.type == sf::Event::MouseButtonPressed) {
            if (event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                int rank = floor(float(mousePos.y)/112.5f);
                int file = floor(float(mousePos.x)/112.5f);
                if (rank <= 7 && file <= 7) {
                    if (startRank == 8) {
                        if (validMoves->GetState(rank, file) != EMPTY) {
                            startRank = rank;
                            startFile = file;
                            validMoves = board.validMoves(rank, file);
                        }
                    } else {
                        validMoves = board.moveValid(startRank, startFile, rank, file, validMoves);
                        startRank = 8;
                        startFile = 8;
                    }
                }
            }
        }

        window.clear();

        for (int rank = 0; rank < 8; rank++) {
            for (int file = 0; file < 8; file++) {
                if (board.pieceTargeted(board.board, rank, file, false)) {
                    background.setFillColor(red);
                } else if (rank == startRank && file == startFile) {
                    background.setFillColor(yellow);
                } else if (rank%2 == 0 xor file%2 == 0) {
                    background.setFillColor(green);
                } else {
                    background.setFillColor(white);
                }
                background.setPosition(float(file)*112.5f, float(rank)*112.5f);
                window.draw(background);

                pieceMarker->setTexture(textureMap);
                float x;
                float y;
                SquareState type = board.board->GetState(rank, file);
                if (type != EMPTY and type != VALID_MOVE and type != VALID_CAPTURE) {
                    switch (type) {
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

                    pieceMarker->setTextureRect(sf::IntRect(int(x*textureSize.x/6), int(y*textureSize.y/2), int(textureSize.x)/6, int(textureSize.y)/2));
                    pieceMarker->setPosition(float(file+0.1)*112.5f, float(rank+0.1)*112.5f);
                    window.draw(*pieceMarker);
                }


                if (validMoves->GetState(rank, file) == VALID_MOVE) {
                    moveMarker.setRadius(0.00001);
                    moveMarker.setOutlineThickness(18);
                    moveMarker.setPosition(float(file)*112.5f+56.25f, float(rank)*112.5f+56.25f);
                    window.draw(moveMarker);
                } else if (validMoves->GetState(rank, file) == VALID_CAPTURE) {
                    moveMarker.setRadius(46.25);
                    moveMarker.setOutlineThickness(10);
                    moveMarker.setPosition(float(file)*112.5f+10, float(rank)*112.5f+10);
                    window.draw(moveMarker);
                }
            }
        }

        window.draw(text);
        window.display();
    }
    return 0;
}
