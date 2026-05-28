//
// Created by Codex on 5/27/26.
//

#ifndef MOVENOTATION_H
#define MOVENOTATION_H

#include <string>

#include "Board.h"

struct MoveRecord {
    Move move;
    std::string san;
    Piece capturedPiece = EMPTY;
};

inline std::string positionToNotation(Position position) {
    int rank = position.rank();
    int file = position.file();

    char files[8] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};

    return files[file] + std::to_string(rank + 1);
}

inline char pieceLetter(Piece piece) {
    switch (piece) {
        case WHITE_KNIGHT:
        case BLACK_KNIGHT: return 'N';

        case WHITE_BISHOP:
        case BLACK_BISHOP: return 'B';

        case WHITE_ROOK:
        case BLACK_ROOK: return 'R';

        case WHITE_QUEEN:
        case BLACK_QUEEN: return 'Q';

        case WHITE_KING:
        case BLACK_KING: return 'K';

        default:
            return '\0';
    }
}

inline std::string moveToSAN(const Board& board, Move move) {
    Piece piece = board.getPiece(move.starting());

    if (move.isCastleKing())  return "O-O";
    if (move.isCastleQueen()) return "O-O-O";

    std::string san;
    char letter = pieceLetter(piece);

    if (letter != '\0') {
        san += letter;

        bool sameFile = false, sameRank = false, ambiguous = false;

        for (Position pos; pos <= Position(7, 7); pos++) {
            if (pos == move.starting()) continue;
            if (board.getPiece(pos) != piece) continue;

            Board tempBoard(board);
            MoveList pieceMoves;
            tempBoard.getValidMoves(pos, pieceMoves);

            for (int i = 0; i < pieceMoves.count; i++) {
                if (pieceMoves[i].target() == move.target()) {
                    ambiguous = true;
                    if (pos.file() == move.starting().file()) sameFile = true;
                    if (pos.rank() == move.starting().rank()) sameRank = true;
                    break;
                }
            }
        }

        if (ambiguous) {
            if (!sameFile) {
                san += char('a' + move.starting().file());
            } else if (!sameRank) {
                san += char('1' + move.starting().rank());
            } else {
                san += char('a' + move.starting().file());
                san += char('1' + move.starting().rank());
            }
        }
    }

    if (move.isCapture()) {
        if (letter == '\0') {
            san += char('a' + move.starting().file());
        }
        san += 'x';
    }

    san += positionToNotation(move.target());

    if ((piece == WHITE_PAWN && move.target().rank() == 7) ||
        (piece == BLACK_PAWN && move.target().rank() == 0)) {
        san += '=';
        san += pieceLetter(WHITE_QUEEN);
    }

    return san;
}

inline void appendCheckSuffix(std::string& san, Board& boardAfterMove) {
    if (boardAfterMove.checkMate()) {
        san += '#';
    }
    else if (boardAfterMove.check(boardAfterMove.getPlayerTurn())) {
        san += '+';
    }
}

#endif // MOVENOTATION_H
