//
// Created by Andreas Royset on 11/30/24.
//

#ifndef BOARDSTATE_H
#define BOARDSTATE_H

enum SquareState {
    BLACK_KING=-7,
    BLACK_QUEEN=-6,
    BLACK_ROOK=-5,
    BLACK_BISHOP=-4,
    BLACK_KNIGHT=-3,
    BLACK_PAWN=-2,
    VALID_MOVE=-1,
    EMPTY=0,
    VALID_CAPTURE=1,
    WHITE_PAWN=2,
    WHITE_KNIGHT=3,
    WHITE_BISHOP=4,
    WHITE_ROOK=5,
    WHITE_QUEEN=6,
    WHITE_KING=7

};

class BoardState {
    private:
    SquareState board[8][8];
    public:
    BoardState(){
        board[0][0]=BLACK_ROOK;
        board[0][1]=BLACK_KNIGHT;
        board[0][2]=BLACK_BISHOP;
        board[0][3]=BLACK_QUEEN;
        board[0][4]=BLACK_KING;
        board[0][5]=BLACK_BISHOP;
        board[0][6]=BLACK_KNIGHT;
        board[0][7]=BLACK_ROOK;
        for(int i = 0; i < 8; i++){
            board[1][i] = BLACK_PAWN;
        }

        for(int i = 2; i < 6; i++){
            for(int j = 0; j < 8; j++){
                board[i][j] = EMPTY;
            }
        }

        for(int i = 0; i < 8; i++){
            board[6][i] = WHITE_PAWN;
        }
        board[7][0]=WHITE_ROOK;
        board[7][1]=WHITE_KNIGHT;
        board[7][2]=WHITE_BISHOP;
        board[7][3]=WHITE_QUEEN;
        board[7][4]=WHITE_KING;
        board[7][5]=WHITE_BISHOP;
        board[7][6]=WHITE_KNIGHT;
        board[7][7]=WHITE_ROOK;
    }

    explicit BoardState(const BoardState *reference){
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                this->board[i][j] = reference->board[i][j];
            }
        }
    }

    void move(const int startRank, const int startFile, const int targetRank, const int targetFile) {
        board[targetRank][targetFile] = board[startRank][startFile];
        board[startRank][startFile] = EMPTY;
    }

    void ChangeState(const int i, const int j, const SquareState state) {
        if (i < 0 or i > 7 or j < 0 or j > 7) {
            std::cout << "Invalid position in BoardState.ChangeState()" << std::endl;
        }
        this->board[i][j] = state;
    }

    [[nodiscard]] SquareState GetState(const int i, const int j) const {
        return this->board[i][j];
    }

};

#endif //BOARDSTATE_H
