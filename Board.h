//
// Created by Andreas Royset on 11/28/24.
//

#ifndef BOARD_H
#define BOARD_H

#include <vector>
#include "BoardState.h"

inline bool posIsValid(const int rank, const int file, const BoardState* state) {
    return state->GetState(rank, file) == VALID_MOVE || state->GetState(rank, file) == VALID_CAPTURE;
}

class Board {
    public:
    BoardState* board{};
    Board() {
        this->board = new BoardState();
    }
    ~Board() {
        delete board;
    }

    [[nodiscard]] bool KingInCheck(const BoardState* checkBoard, bool const WhiteKing) const {
        int kingRank = 0;
        int kingFile = 0;

        for (int rank = 0; rank < 8; rank++) {
            for (int file = 0; file < 8; file++) {
                SquareState square = checkBoard->GetState(rank, file);
                if ((square == WHITE_KING and WhiteKing) or (square == BLACK_KING and !WhiteKing)) {
                    kingRank = rank;
                    kingFile = file;
                }
            }
        }

        if (pieceTargeted(checkBoard, kingRank, kingFile, WhiteKing)) {
            return true;
        }
        return false;

    }

    [[nodiscard]] bool pieceTargeted(const BoardState* checkBoard, const int Rank, const int File, const bool PieceIsWhite) const {
        for (int rank = 0; rank < 8; rank++) {
            for (int file = 0; file < 8; file++) {
                if((checkBoard->GetState(rank, file) > 1 and !PieceIsWhite) or (checkBoard->GetState(rank, file) < -1 and PieceIsWhite)) {
                    BoardState* moves = findMoves(rank, file);
                    if (moves->GetState(Rank, File) == VALID_CAPTURE or moves->GetState(Rank, File) == VALID_MOVE) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    [[nodiscard]] bool checkMate(bool isWhite, int enPassant) const {
        if (KingInCheck(board, isWhite)) {
            return true;
        }
        return false;
    }

    [[nodiscard]] BoardState* findMoves(const int Rank, const int File) const {
        auto* moves = new BoardState(this->board);
        SquareState square_state = board->GetState(Rank, File);
        bool isWhite = square_state > 1;
        switch (abs(square_state)) {
            case WHITE_KING: {
                for (int rank = Rank-1; rank <= Rank+1; rank++) {
                    for (int file = File-1; file <= File+1; file++) {
                        if (0 <= rank and rank < 8 and 0 <= file and file < 8) {
                            int squareState = board->GetState(rank, file);
                            if (squareState == EMPTY) {
                                moves->ChangeState(rank, file, VALID_MOVE);
                            } else if ((squareState < -1 and isWhite) or (squareState > 1 and !isWhite)) {
                                moves->ChangeState(rank, file, VALID_CAPTURE);
                            }
                        }
                    }
                }
                break;
            }
            case WHITE_QUEEN: {
                for (int i = Rank-1; i >= 0; i--) {
                    int squareState = board->GetState(i, File);
                    if (squareState == EMPTY) {
                        moves->ChangeState(i, File, VALID_MOVE);
                    } else if ((squareState < -1 and isWhite) or (squareState > 1 and !isWhite)) {
                        moves->ChangeState(i, File, VALID_CAPTURE);
                        break;
                    } else {break;}
                }
                for (int i = Rank+1; i <= 7; i++) {
                    int squareState = board->GetState(i, File);
                    if (squareState == EMPTY) {
                        moves->ChangeState(i, File, VALID_MOVE);
                    } else if ((squareState < -1 and isWhite) or (squareState > 1 and !isWhite)) {
                        moves->ChangeState(i, File, VALID_CAPTURE);
                        break;
                    } else {break;}
                }
                for (int i = File-1; i >= 0; i--) {
                    int squareState = board->GetState(Rank, i);
                    if (squareState == EMPTY) {
                        moves->ChangeState(Rank, i, VALID_MOVE);
                    } else if ((squareState < -1 and isWhite) or (squareState > 1 and !isWhite)) {
                        moves->ChangeState(Rank, i, VALID_CAPTURE);
                        break;
                    } else {break;}
                }
                for (int i = File+1; i <= 7; i++) {
                    int squareState = board->GetState(Rank, i);
                    if (squareState == EMPTY) {
                        moves->ChangeState(Rank, i, VALID_MOVE);
                    } else if ((squareState < -1 and isWhite) or (squareState > 1 and !isWhite)) {
                        moves->ChangeState(Rank, i, VALID_CAPTURE);
                        break;
                    } else {break;}
                }
                int maxD = std::min(Rank, File);
                for (int i = 1; i <= maxD; i++) {
                    int squareState = board->GetState(Rank-i, File-i);
                    if (squareState == EMPTY) {
                        moves->ChangeState(Rank-i, File-i, VALID_MOVE);
                    } else if ((squareState < -1 and isWhite) or (squareState > 1 and !isWhite)) {
                        moves->ChangeState(Rank-i, File-i, VALID_CAPTURE);
                        break;
                    } else {break;}
                }
                maxD = std::min(int(Rank), 7-File);
                for (int i = 1; i <= maxD; i++) {
                    int squareState = board->GetState(Rank-i, File+i);
                    if (squareState == EMPTY) {
                        moves->ChangeState(Rank-i, File+i, VALID_MOVE);
                    } else if ((squareState < -1 and isWhite) or (squareState > 1 and !isWhite)) {
                        moves->ChangeState(Rank-i, File+i, VALID_CAPTURE);
                        break;
                    } else {break;}
                }
                maxD = std::min(7-Rank, 7-File);
                for (int i = 1; i <= maxD; i++) {
                    int squareState = board->GetState(Rank+i, File+i);
                    if (squareState == EMPTY) {
                        moves->ChangeState(Rank+i, File+i, VALID_MOVE);
                    } else if ((squareState < -1 and isWhite) or (squareState > 1 and !isWhite)) {
                        moves->ChangeState(Rank+i, File+i, VALID_CAPTURE);
                        break;
                    } else {break;}
                }
                maxD = std::min(7-Rank, int(File));
                for (int i = 1; i <= maxD; i++) {
                    int squareState = board->GetState(Rank+i, File-i);
                    if (squareState == EMPTY) {
                        moves->ChangeState(Rank+i, File-i, VALID_MOVE);
                    } else if ((squareState < -1 and isWhite) or (squareState > 1 and !isWhite)) {
                        moves->ChangeState(Rank+i, File-i, VALID_CAPTURE);
                        break;
                    } else {break;}
                }
                break;
            }
            case WHITE_ROOK: {
                for (int i = Rank-1; i >= 0; i--) {
                    int squareState = board->GetState(i, File);
                    if (squareState == EMPTY) {
                        moves->ChangeState(i, File, VALID_MOVE);
                    } else if ((squareState < -1 and isWhite) or (squareState > 1 and !isWhite)) {
                        moves->ChangeState(i, File, VALID_CAPTURE);
                        break;
                    } else {break;}
                }
                for (int i = Rank+1; i <= 7; i++) {
                    int squareState = board->GetState(i, File);
                    if (squareState == EMPTY) {
                        moves->ChangeState(i, File, VALID_MOVE);
                    } else if ((squareState < -1 and isWhite) or (squareState > 1 and !isWhite)) {
                        moves->ChangeState(i, File, VALID_CAPTURE);
                        break;
                    } else {break;}
                }
                for (int i = File-1; i >= 0; i--) {
                    int squareState = board->GetState(Rank, i);
                    if (squareState == EMPTY) {
                        moves->ChangeState(Rank, i, VALID_MOVE);
                    } else if ((squareState < -1 and isWhite) or (squareState > 1 and !isWhite)) {
                        moves->ChangeState(Rank, i, VALID_CAPTURE);
                        break;
                    } else {break;}
                }
                for (int i = File+1; i <= 7; i++) {
                    int squareState = board->GetState(Rank, i);
                    if (squareState == EMPTY) {
                        moves->ChangeState(Rank, i, VALID_MOVE);
                    } else if ((squareState < -1 and isWhite) or (squareState > 1 and !isWhite)) {
                        moves->ChangeState(Rank, i, VALID_CAPTURE);
                        break;
                    } else {break;}
                }
                break;
            }
            case WHITE_BISHOP: {
                int maxD = std::min(Rank, File);
                for (int i = 1; i <= maxD; i++) {
                    int squareState = board->GetState(Rank-i, File-i);
                    if (squareState == EMPTY) {
                        moves->ChangeState(Rank-i, File-i, VALID_MOVE);
                    } else if ((squareState < -1 and isWhite) or (squareState > 1 and !isWhite)) {
                        moves->ChangeState(Rank-i, File-i, VALID_CAPTURE);
                        break;
                    } else {break;}
                }
                maxD = std::min(int(Rank), 7-File);
                for (int i = 1; i <= maxD; i++) {
                    int squareState = board->GetState(Rank-i, File+i);
                    if (squareState == EMPTY) {
                        moves->ChangeState(Rank-i, File+i, VALID_MOVE);
                    } else if ((squareState < -1 and isWhite) or (squareState > 1 and !isWhite)) {
                        moves->ChangeState(Rank-i, File+i, VALID_CAPTURE);
                        break;
                    } else {break;}
                }
                maxD = std::min(7-Rank, 7-File);
                for (int i = 1; i <= maxD; i++) {
                    int squareState = board->GetState(Rank+i, File+i);
                    if (squareState == EMPTY) {
                        moves->ChangeState(Rank+i, File+i, VALID_MOVE);
                    } else if ((squareState < -1 and isWhite) or (squareState > 1 and !isWhite)) {
                        moves->ChangeState(Rank+i, File+i, VALID_CAPTURE);
                        break;
                    } else {break;}
                }
                maxD = std::min(7-Rank, int(File));
                for (int i = 1; i <= maxD; i++) {
                    int squareState = board->GetState(Rank+i, File-i);
                    if (squareState == EMPTY) {
                        moves->ChangeState(Rank+i, File-i, VALID_MOVE);
                    } else if ((squareState < -1 and isWhite) or (squareState > 1 and !isWhite)) {
                        moves->ChangeState(Rank+i, File-i, VALID_CAPTURE);
                        break;
                    } else {break;}
                }
                break;
            }
            case WHITE_KNIGHT: {
                if (Rank >= 2 and File >= 1) {
                    int squareState = board->GetState(Rank-2, File-1);
                    if (squareState == EMPTY) {
                        moves->ChangeState(Rank-2, File-1, VALID_MOVE);
                    } else if((squareState < -1 and isWhite) or (squareState > 1 and !isWhite)) {
                        moves->ChangeState(Rank-2, File-1, VALID_CAPTURE);
                    }
                }
                if (Rank >= 2 and File <= 6) {
                    int squareState = board->GetState(Rank-2, File+1);
                    if (squareState == EMPTY) {
                        moves->ChangeState(Rank-2, File+1, VALID_MOVE);
                    } else if ((squareState < -1 and isWhite) or (squareState > 1 and !isWhite)) {
                        moves->ChangeState(Rank-2, File+1, VALID_CAPTURE);
                    }
                }
                if (Rank >= 1 and File <= 5) {
                    int squareState = board->GetState(Rank-1, File+2);
                    if (squareState == EMPTY) {
                        moves->ChangeState(Rank-1, File+2, VALID_MOVE);
                    } else if ((squareState < -1 and isWhite) or (squareState > 1 and !isWhite)) {
                        moves->ChangeState(Rank-1, File+2, VALID_CAPTURE);
                    }
                }
                if (Rank <= 6 and File <= 5) {
                    int squareState = board->GetState(Rank+1, File+2);
                    if (squareState == EMPTY) {
                        moves->ChangeState(Rank+1, File+2, VALID_MOVE);
                    } else if ((squareState < -1 and isWhite) or (squareState > 1 and !isWhite)) {
                        moves->ChangeState(Rank+1, File+2, VALID_CAPTURE);
                    }
                }
                if (Rank <= 5 and File <= 6) {
                    int squareState = board->GetState(Rank+2, File+1);
                    if (squareState == EMPTY) {
                        moves->ChangeState(Rank+2, File+1, VALID_MOVE);
                    } else if ((squareState < -1 and isWhite) or (squareState > 1 and !isWhite)) {
                        moves->ChangeState(Rank+2, File+1, VALID_CAPTURE);
                    }
                }
                if (Rank <= 5 and File >= 1) {
                    int squareState = board->GetState(Rank+2, File-1);
                    if (squareState == EMPTY) {
                        moves->ChangeState(Rank+2, File-1, VALID_MOVE);
                    } else if ((squareState < -1 and isWhite) or (squareState > 1 and !isWhite)) {
                        moves->ChangeState(Rank+2, File-1, VALID_CAPTURE);
                    }
                }
                if (Rank <= 6 and File >= 2) {
                    int squareState = board->GetState(Rank+1, File-2);
                    if (squareState == EMPTY) {
                        moves->ChangeState(Rank+1, File-2, VALID_MOVE);
                    } else if ((squareState < -1 and isWhite) or (squareState > 1 and !isWhite)) {
                        moves->ChangeState(Rank+1, File-2, VALID_CAPTURE);
                    }
                }
                if (Rank >= 1 and File >= 2) {
                    int squareState = board->GetState(Rank-1, File-2);
                    if (squareState == EMPTY) {
                        moves->ChangeState(Rank-1, File-2, VALID_MOVE);
                    } else if ((squareState < -1 and isWhite) or (squareState > 1 and !isWhite)) {
                        moves->ChangeState(Rank-1, File-2, VALID_CAPTURE);
                    }
                }
                break;
            }
            case WHITE_PAWN: {
                if (square_state > 1) {
                    if (Rank != 0) {
                        if (board->GetState(Rank-1, File) == EMPTY) {
                            moves->ChangeState(Rank-1, File, VALID_MOVE);
                            if (Rank == 6 and board->GetState(Rank-2, File) == EMPTY) {
                                moves->ChangeState(Rank-2, File, VALID_MOVE);
                            }
                        }
                        if (File != 0 and board->GetState(Rank-1, File-1) < -1) {
                            moves->ChangeState(Rank-1, File-1, VALID_CAPTURE);
                        }
                        if (File != 7 and board->GetState(Rank-1, File+1) < -1) {
                            moves->ChangeState(Rank-1, File+1, VALID_CAPTURE);
                        }
                    }
                } else if (square_state < -1) {
                    if (Rank != 7) {
                        if (board->GetState(Rank+1, File) == EMPTY) {
                            moves->ChangeState(Rank+1, File, VALID_MOVE);
                            if (Rank == 1 and board->GetState(Rank+2, File) == EMPTY) {
                                moves->ChangeState(Rank+2, File, VALID_MOVE);
                            }
                        }
                        if (File != 0 and board->GetState(Rank+1, File-1) > 1) {
                            moves->ChangeState(Rank+1, File-1, VALID_CAPTURE);
                        }
                        if (File != 7 and board->GetState(Rank+1, File+1) > 1) {
                            moves->ChangeState(Rank+1, File+1, VALID_CAPTURE);
                        }
                    }
                }
                break;
            }
            default: {break;}
        }
        return moves;
    }

    [[nodiscard]] BoardState* validMoves(const int Rank, const int File) const {
        BoardState* moves = findMoves(Rank, File);
        bool isWhite = board->GetState(Rank, File) > 1;

        for (int rank = 0; rank < 8; ++rank) {
            for (int file = 0; file < 8; ++file) {
                if (moves->GetState(rank, file) == VALID_MOVE or moves->GetState(rank, file) == VALID_CAPTURE) {
                    auto* testBoard = new BoardState(board);
                    testBoard->move(Rank, File, rank, file);
                    if (KingInCheck(testBoard, isWhite)) {
                        moves->ChangeState(rank, file, board->GetState(rank, file));
                    }
                    delete testBoard;
                }
            }
        }
        return moves;
    }

    void move(const int startRank, const int startFile, const int targetRank, const int targetFile) const {
        board->move(startRank, startFile, targetRank, targetFile);
    }

    [[nodiscard]] BoardState* moveValid(int const startRank, const int startFile, const int targetRank, const int targetFile, BoardState* validState) const {
        SquareState piece = board->GetState(startRank, startFile);
        if (posIsValid(targetRank, targetFile, validState)) {
            if (piece == WHITE_PAWN) {
                if (targetRank == 0) {
                    board->ChangeState(startRank, startFile, WHITE_QUEEN);
                }
            } else if (piece == BLACK_PAWN) {
                if (targetRank == 0) {
                    board->ChangeState(startRank, startFile, BLACK_QUEEN);
                }
            }
            move(startRank, startFile, targetRank, targetFile);
        }
        return this->board;
    }
};

#endif //BOARD_H
