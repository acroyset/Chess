//
// Created by Andreas Royset on 11/30/24.
//

#ifndef BOARD_H
#define BOARD_H
#include "BoardState.h"
#include "../AI/OpeningBook.h"
#include "offsets.h"
#include <sstream>

float tableMg(Piece p, int idx);
float tableEg(Piece p, int idx);
int gamePhaseWeight(Piece p);

// Maximum game length for fixed-size history stack
static constexpr int MAX_HISTORY = 512;

class Board{
    BoardState state;

    // Fixed-size stack — eliminates all heap allocation during search
    BoardState stateStack[MAX_HISTORY];
    int stackTop = 0;

    // Separate vector only for repetition detection across the root
    // (search threads share the stack; root history is small)
    std::vector<uint64_t> rootHashes;

public:

    Board();
    explicit Board(const std::string& fen);
    Board(const Board& other);
    void set(const Board &other);

    [[nodiscard]] Piece getPiece(Position position) const;
    [[nodiscard]] PieceType getPieceType(Position position) const;
    [[nodiscard]] bool getPlayerTurn() const {return state.playerTurn;}
    [[nodiscard]] bool canCastleKingSide(bool black) const {return black ? state.blackCastleKing : state.whiteCastleKing;}
    [[nodiscard]] bool canCastleQueenSide(bool black) const {return black ? state.blackCastleQueen : state.whiteCastleQueen;}
    [[nodiscard]] float getWhiteMg() const {return state.whiteMg;}
    [[nodiscard]] float getWhiteEg() const {return state.whiteEg;}
    [[nodiscard]] float getBlackMg() const {return state.blackMg;}
    [[nodiscard]] float getBlackEg() const {return state.blackEg;}
    [[nodiscard]] int getEvalPhase() const {return state.evalPhase;}
    [[nodiscard]] uint32_t getRankBits(int rank) const {return state.board[rank];}
    [[nodiscard]] uint64_t getPieceBits(Piece piece) const {return state.pieceBits[piece];}

    // --- Move generation using pieceBits iteration ---

    void getValidMoves(MoveList& validMoves);
    void getCaptureMoves(MoveList& validMoves);
    void getValidMoves(Position position, MoveList& validMoves);

    [[nodiscard]] bool check(bool player) const;
    [[nodiscard]] bool moveGivesCheck(Move move) const;
    [[nodiscard]] bool checkMate() {return noMoves(state.playerTurn) && check(state.playerTurn);}
    [[nodiscard]] bool draw() {
        if (draw50Move()) return true;
        if (drawRepetition()) return true;
        if (drawInsufficientMaterial()) return true;
        if (staleMate(state.playerTurn)) return true;

        return false;
    }
    [[nodiscard]] bool draw50Move() const {return state.lastPawnMoveOrCapture >= 100;}
    [[nodiscard]] bool drawInsufficientMaterial() const;
    [[nodiscard]] bool drawRepetition() const;
    [[nodiscard]] bool staleMate(const bool player) {return noMoves(player) && !check(player);}

    bool move(const Move& move);
    // Null move: flip side to move, clear EP, update hash
    // Used by null move pruning in search
    void makeNullMove();
    void undoMove();

    // Called at root before search starts — records game history for repetition detection
    void setRootHashes(const std::vector<uint64_t>& hashes) {rootHashes = hashes;}

    [[nodiscard]] uint64_t computeHash() const;
    [[nodiscard]] uint64_t getHash() const {return state.hash;}

    [[nodiscard]] uint64_t computePolyglotHash() const;

private:

    static int popLsb(uint64_t& bits) {
        int sq = __builtin_ctzll(bits);
        bits &= bits - 1;
        return sq;
    }

    [[nodiscard]] uint64_t getFriendlyBits() const;

    // increment eval
    void clearIncrementalEval();
    void addPieceToEval(Piece piece, Position position);
    void removePieceFromEval(Piece piece, Position position);
    void refreshIncrementalEval();
    void refreshPieceBits();

    void resetBoard();
    void movePiece(Position starting, Position target, Piece piece);
    void clear(Position position);
    void set(Position position, Piece piece);

    void getPossibleMoves(Position position, MoveList& moves) const;
    void getPossibleCaptures(Position position, MoveList& moves) const;

    [[nodiscard]] bool squareAttacked(Position square, bool byBlack) const;

    [[nodiscard]] bool validMove(const Move& move);

    [[nodiscard]] bool noMoves(bool player);

    [[nodiscard]] Position findKing(bool player) const;

    void switchPlayer() {state.playerTurn = !state.playerTurn;}
};

#endif //BOARD_H
