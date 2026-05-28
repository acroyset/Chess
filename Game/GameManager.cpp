//
// Created by Codex on 5/27/26.
//

#include "GameManager.h"

#include "../Stockfish/StockfishPlayer.h"
#include "FenSaveLoader.h"

GameManager::GameManager(std::unique_ptr<Player> whitePlayer, std::unique_ptr<Player> blackPlayer)
    : GameManager(Board(), std::move(whitePlayer), std::move(blackPlayer)) {}

GameManager::GameManager(const Board& board, std::unique_ptr<Player> whitePlayer, std::unique_ptr<Player> blackPlayer)
    : board(board), whitePlayer(std::move(whitePlayer)), blackPlayer(std::move(blackPlayer)) {
    gameStartFen = boardToFEN(this->board);
    syncStockfishHistory();
}

GameAdvanceResult GameManager::runGame(float deltaTime, bool selectPlayerMove) {
    GameAdvanceResult result;

    if (!isGameOver()) {
        currentPlayer()->tickClock(deltaTime);
    }

    refreshFlaggedState();
    result.gameOver = isGameOver();

    if (result.gameOver || !selectPlayerMove) {
        return result;
    }

    std::optional<Move> chosenMove = currentPlayer()->selectMove(board);
    if (!chosenMove.has_value()) {
        return result;
    }

    return applyMove(chosenMove.value());
}

GameAdvanceResult GameManager::applyMove(Move move) {
    GameAdvanceResult result;
    result.gameOver = isGameOver();
    if (result.gameOver) {
        return result;
    }

    std::optional<Move> legalMove = normalizeMove(move);
    if (!legalMove.has_value()) {
        result.invalidMove = true;
        return result;
    }

    Player* movingPlayer = currentPlayer();
    Move resolvedMove = legalMove.value();
    std::string san = moveToSAN(board, resolvedMove);
    Piece capturedPiece = capturedPieceForMove(resolvedMove);

    if (!board.move(resolvedMove)) {
        result.invalidMove = true;
        return result;
    }

    movingPlayer->incrementTime();
    if (auto* ai = dynamic_cast<AIPlayer*>(movingPlayer)) {
        lastCompletedAi = ai;
    }

    appendCheckSuffix(san, board);
    moveHistory.push_back({resolvedMove, san, capturedPiece});
    syncStockfishHistory();
    refreshFlaggedState();

    result.moveApplied = true;
    result.gameOver = isGameOver();
    return result;
}

bool GameManager::undoMove() {
    if (moveHistory.empty()) {
        return false;
    }

    board.undoMove();
    moveHistory.pop_back();
    lastCompletedAi = nullptr;
    resetPlayerInput();
    syncStockfishHistory();
    refreshFlaggedState();
    return true;
}

bool GameManager::loadFen(const std::string& fen) {
    if (fen.empty() || fen.find('/') == std::string::npos) {
        return false;
    }

    try {
        Board newBoard(fen);
        board = newBoard;
        moveHistory.clear();
        gameStartFen = boardToFEN(board);
        lastCompletedAi = nullptr;
        whiteFlagged = false;
        blackFlagged = false;
        resetPlayerInput();
        syncStockfishHistory();
        return true;
    } catch (...) {
        return false;
    }
}

void GameManager::resetPlayerInput() const {
    whitePlayer->resetInput();
    blackPlayer->resetInput();
}

Player* GameManager::currentPlayer() {
    return board.getPlayerTurn() ? blackPlayer.get() : whitePlayer.get();
}

const Player* GameManager::currentPlayer() const {
    return board.getPlayerTurn() ? blackPlayer.get() : whitePlayer.get();
}

bool GameManager::isGameOver() {
    refreshFlaggedState();
    return board.checkMate() || board.draw() || whiteFlagged || blackFlagged;
}

std::optional<Move> GameManager::normalizeMove(Move move) {
    MoveList legalMoves;
    board.getValidMoves(legalMoves);

    for (int i = 0; i < legalMoves.count; i++) {
        Move legalMove = legalMoves[i];
        if (legalMove.starting() == move.starting() && legalMove.target() == move.target()) {
            return legalMove;
        }
    }

    return std::nullopt;
}

Piece GameManager::capturedPieceForMove(Move move) const {
    if (move.isEnPassant()) {
        Piece movingPiece = board.getPiece(move.starting());
        return movingPiece == WHITE_PAWN ? BLACK_PAWN : WHITE_PAWN;
    }

    return board.getPiece(move.target());
}

std::vector<Move> GameManager::playedMoves() const {
    std::vector<Move> moves;
    moves.reserve(moveHistory.size());
    for (const MoveRecord& record : moveHistory) {
        moves.push_back(record.move);
    }
    return moves;
}

void GameManager::syncStockfishHistory() const {
    std::vector<Move> moves = playedMoves();
    if (auto* stockfishPlayer = dynamic_cast<StockfishPlayer*>(whitePlayer.get())) {
        stockfishPlayer->setGameHistory(gameStartFen, moves);
    }
    if (auto* stockfishPlayer = dynamic_cast<StockfishPlayer*>(blackPlayer.get())) {
        stockfishPlayer->setGameHistory(gameStartFen, moves);
    }
}

void GameManager::refreshFlaggedState() {
    whiteFlagged = whitePlayer->outOfTime();
    blackFlagged = blackPlayer->outOfTime();
}
