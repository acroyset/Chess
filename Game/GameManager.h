//
// Created by Codex on 5/27/26.
//

#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../AI/AIPlayer.h"
#include "../Player.h"
#include "Board.h"
#include "MoveNotation.h"

struct GameAdvanceResult {
    bool moveApplied = false;
    bool invalidMove = false;
    bool gameOver = false;
};

class GameManager {
    Board board;
    std::unique_ptr<Player> whitePlayer;
    std::unique_ptr<Player> blackPlayer;
    std::vector<MoveRecord> moveHistory;
    AIPlayer* lastCompletedAi = nullptr;
    std::string gameStartFen;
    bool whiteFlagged = false;
    bool blackFlagged = false;

public:
    GameManager(std::unique_ptr<Player> whitePlayer, std::unique_ptr<Player> blackPlayer);
    GameManager(const Board& board, std::unique_ptr<Player> whitePlayer, std::unique_ptr<Player> blackPlayer);

    GameAdvanceResult runGame(float deltaTime, bool selectPlayerMove = true);
    bool undoMove();
    bool loadFen(const std::string& fen);

    void resetPlayerInput() const;

    [[nodiscard]] Board& getBoard() { return board; }
    [[nodiscard]] const Board& getBoard() const { return board; }
    [[nodiscard]] Player* getWhitePlayer() { return whitePlayer.get(); }
    [[nodiscard]] Player* getBlackPlayer() { return blackPlayer.get(); }
    [[nodiscard]] const Player* getWhitePlayer() const { return whitePlayer.get(); }
    [[nodiscard]] const Player* getBlackPlayer() const { return blackPlayer.get(); }
    [[nodiscard]] Player* currentPlayer();
    [[nodiscard]] const Player* currentPlayer() const;
    [[nodiscard]] const std::vector<MoveRecord>& getMoveHistory() const { return moveHistory; }
    [[nodiscard]] const MoveRecord* getLastMoveRecord() const { return moveHistory.empty() ? nullptr : &moveHistory.back(); }
    [[nodiscard]] Move getLastMove() const { return moveHistory.empty() ? Move() : moveHistory.back().move; }
    [[nodiscard]] MoveList getShownMoves() const { return currentPlayer()->getShownMoves(); }
    [[nodiscard]] AIPlayer* getLastCompletedAi() const { return lastCompletedAi; }
    [[nodiscard]] bool isWhiteFlagged() const { return whiteFlagged; }
    [[nodiscard]] bool isBlackFlagged() const { return blackFlagged; }
    [[nodiscard]] bool canUndoMove() const { return !moveHistory.empty(); }
    [[nodiscard]] bool isGameOver();

private:
    GameAdvanceResult applyMove(Move move);

    [[nodiscard]] std::optional<Move> normalizeMove(Move move);
    [[nodiscard]] Piece capturedPieceForMove(Move move) const;
    [[nodiscard]] std::vector<Move> playedMoves() const;

    void syncStockfishHistory() const;
    void refreshFlaggedState();
};

#endif // GAMEMANAGER_H
