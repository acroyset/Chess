//
// Created by Andreas Royset on 5/21/26.
//

#ifndef HUMANPLAYER_H
#define HUMANPLAYER_H

#include "Player.h"

class HumanPlayer : public Player {
    Position selectedPosition;
    bool pressedLast = false;
    MoveList validMoves;
    float tileSize;

public:
    explicit HumanPlayer(float tileSize)
        : tileSize(tileSize) {
        selectedPosition.setNone();
    }

    std::optional<Move> selectMove(Board& board, sf::RenderWindow& window) override {
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
            if (!pressedLast) {
                pressedLast = true;

                sf::Vector2i localPos = sf::Mouse::getPosition(window);

                int rank = 7 - int(std::floor(float(localPos.y) / tileSize));
                int file = int(std::floor(float(localPos.x) / tileSize));

                if (rank < 0 || rank >= 8 || file < 0 || file >= 8) {
                    selectedPosition.setNone();
                    validMoves.clear();
                    return std::nullopt;
                }

                Position position{rank, file};

                for (int i = 0; i < validMoves.count; i++) {
                    Move move = validMoves[i];
                    if (position == move.target()) {
                        selectedPosition.setNone();
                        validMoves.clear();
                        return move;
                    }
                }

                validMoves.clear();
                if (selectedPosition == position) {
                    selectedPosition.setNone();
                } else if ((board.getPiece(position) >> 3) == board.getPlayerTurn()) {
                    selectedPosition = position;
                    board.getValidMoves(position, validMoves);
                }
            }
        } else {
            pressedLast = false;
        }

        return std::nullopt;
    }

    [[nodiscard]] MoveList getShownMoves() const override {
        return validMoves;
    }
};

#endif