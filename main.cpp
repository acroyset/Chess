#include <iostream>
#include <sstream>

#include "AI/AIPlayer.h"
#include "Stockfish/StockfishPlayer.h"
#include "HumanPlayer.h"
#include "Render/MoveDrawer.h"
#include "Render/PieceDrawer.h"
#include "Player.h"
#include "Render/uiDrawer.h"
#include "Game/FenSaveLoader.h"

std::optional<Move> normalizeMove(Board& board, Move move) {
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

Piece capturedPieceForMove(const Board& board, Move move) {
    if (move.isEnPassant()) {
        Piece movingPiece = board.getPiece(move.starting());
        return movingPiece == WHITE_PAWN ? BLACK_PAWN : WHITE_PAWN;
    }

    return board.getPiece(move.target());
}

int main() {

    openingBook.clear();
    loadOpeningBook("Openings/gm2001.bin");
    loadOpeningBook("Openings/komodo.bin");
    loadOpeningBook("Openings/rodent.bin");

    // setup window
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antiAliasingLevel = 8;
    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "Fullscreen Window", sf::Style::Default, sf::State::Fullscreen, settings);
    window.setFramerateLimit(60);

    const sf::ContextSettings actualSettings = window.getSettings();
    if (actualSettings.antiAliasingLevel < settings.antiAliasingLevel) {
        std::cerr << "Requested " << settings.antiAliasingLevel
                  << "x anti-aliasing, got "
                  << actualSettings.antiAliasingLevel << "x\n";
    }

    int width = int(window.getSize().x);
    int height = int(window.getSize().y);
    float tileSize = float(height)/8.0f;
    constexpr float boardX = 0.0f;

    Searcher searcher;
    uiDrawer renderer(width, height, boardX, searcher);

    Board board;
    PieceDrawer pieceDrawer(tileSize, boardX);
    MoveDrawer moveDrawer(tileSize, boardX);

    std::unique_ptr<Player> whitePlayer = std::make_unique<AIPlayer>(&searcher, 15.0f * 60.0f, 10.0f);
    std::unique_ptr<Player> blackPlayer = std::make_unique<AIPlayer>(&searcher, 15.0f * 60.0f, 10.0f);

    bool blackAtBottom = false;

    auto applyBoardOrientation = [&]() {
        if (auto* humanPlayer = dynamic_cast<HumanPlayer*>(whitePlayer.get())) {
            humanPlayer->setBlackAtBottom(blackAtBottom);
        }
        if (auto* humanPlayer = dynamic_cast<HumanPlayer*>(blackPlayer.get())) {
            humanPlayer->setBlackAtBottom(blackAtBottom);
        }
    };
    applyBoardOrientation();

    std::vector<MoveRecord> moveHistory;
    AIPlayer* lastCompletedAi = nullptr;
    std::string gameStartFen = boardToFEN(board);

    auto playedMoves = [&]() {
        std::vector<Move> moves;
        moves.reserve(moveHistory.size());
        for (const MoveRecord& record : moveHistory) {
            moves.push_back(record.move);
        }
        return moves;
    };

    auto syncStockfishHistory = [&]() {
        std::vector<Move> moves = playedMoves();
        if (auto* stockfishPlayer = dynamic_cast<StockfishPlayer*>(whitePlayer.get())) {
            stockfishPlayer->setGameHistory(gameStartFen, moves);
        }
        if (auto* stockfishPlayer = dynamic_cast<StockfishPlayer*>(blackPlayer.get())) {
            stockfishPlayer->setGameHistory(gameStartFen, moves);
        }
    };

    auto restartEvaluation = [&]() {
        searcher.endSearch();
        searcher.startSearch(board);
    };

    syncStockfishHistory();
    restartEvaluation();

    auto resetPlayerInput = [&]() {
        whitePlayer->resetInput();
        blackPlayer->resetInput();
    };

    auto resetHumanInput = [&]() {
        if (auto* humanPlayer = dynamic_cast<HumanPlayer*>(whitePlayer.get())) {
            humanPlayer->resetInput();
        }
        if (auto* humanPlayer = dynamic_cast<HumanPlayer*>(blackPlayer.get())) {
            humanPlayer->resetInput();
        }
    };

    auto loadFenFromClipboard = [&]() {
        std::string fen = pasteFromClipboard();
        if (fen.empty() || fen.find('/') == std::string::npos) return;

        try {
            Board newBoard(fen);
            board = newBoard;
            moveHistory.clear();
            gameStartFen = boardToFEN(board);
            lastCompletedAi = nullptr;
            auto fenCaptures = inferCapturedFromFEN(board);
            renderer.setFenCaptures(fenCaptures);
            resetPlayerInput();
            syncStockfishHistory();
            restartEvaluation();
            std::cerr << "FEN loaded OK\n";
        } catch (...) {
            std::cerr << "Invalid FEN\n";
        }
    };

    auto copyCurrentFen = [&]() {
        std::string fen = boardToFEN(board);
        copyToClipboard(fen);
        std::cerr << "FEN: " << fen << "\n";
    };

    bool analyticsMode = true;
    sf::Clock clock;
    while (window.isOpen()) {
        bool undoRequested = false;
        bool toggleAnalyticsRequested = false;
        bool flipBoardRequested = false;
        bool copyFenRequested = false;
        bool pasteFenRequested = false;
        bool uiControlClicked = false;

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    window.close();
                }
                if (keyPressed->code == sf::Keyboard::Key::A) {
                    toggleAnalyticsRequested = true;
                }

                // F = paste FEN from clipboard and load
                if (keyPressed->code == sf::Keyboard::Key::F) {
                    pasteFenRequested = true;
                }

                // C = copy current FEN to clipboard
                if (keyPressed->code == sf::Keyboard::Key::C) {
                    copyFenRequested = true;
                }

                if (keyPressed->code == sf::Keyboard::Key::B) {
                    flipBoardRequested = true;
                }

                // P = copy PGN to clipboard
                if (keyPressed->code == sf::Keyboard::Key::P) {
                    copyToClipboard(exportPGN(moveHistory));
                    std::cerr << "PGN copied\n";
                }

            }
            if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mousePressed->button == sf::Mouse::Button::Left) {
                    if (renderer.analyticsButtonContains(mousePressed->position)) {
                        toggleAnalyticsRequested = true;
                        uiControlClicked = true;
                    }
                    else if (renderer.undoButtonContains(mousePressed->position)) {
                        undoRequested = true;
                        uiControlClicked = true;
                    }
                    else if (renderer.flipButtonContains(mousePressed->position)) {
                        flipBoardRequested = true;
                        uiControlClicked = true;
                    }
                    else if (renderer.copyFenButtonContains(mousePressed->position)) {
                        copyFenRequested = true;
                        uiControlClicked = true;
                    }
                    else if (renderer.pasteFenButtonContains(mousePressed->position)) {
                        pasteFenRequested = true;
                        uiControlClicked = true;
                    }
                }
            }
        }

        float deltaTime = clock.restart().asSeconds();

        if (toggleAnalyticsRequested) {
            analyticsMode = !analyticsMode;
            renderer.setAnalyticsMode(analyticsMode);
        }

        if (flipBoardRequested) {
            blackAtBottom = !blackAtBottom;
            applyBoardOrientation();
            resetHumanInput();
        }

        if (copyFenRequested) {
            copyCurrentFen();
        }

        if (pasteFenRequested) {
            loadFenFromClipboard();
        }

        bool skipMoveSelection = uiControlClicked || flipBoardRequested || copyFenRequested || pasteFenRequested;
        if (undoRequested && !moveHistory.empty()) {
            board.undoMove();
            moveHistory.pop_back();
            lastCompletedAi = nullptr;
            resetPlayerInput();
            syncStockfishHistory();
            skipMoveSelection = true;

            restartEvaluation();
        }

        window.clear();

        bool checkmate = board.checkMate();
        bool draw = board.draw();
        bool endGame = checkmate || draw;

        Player* currentPlayer = board.getPlayerTurn() ? blackPlayer.get() : whitePlayer.get();
        if (!endGame) {
            currentPlayer->tickClock(deltaTime);
        }

        bool whiteFlagged = whitePlayer->outOfTime();
        bool blackFlagged = blackPlayer->outOfTime();
        bool timeOut = whiteFlagged || blackFlagged;
        endGame = endGame || timeOut;

	    std::optional<Move> chosenMove = std::nullopt;
        if (!endGame && window.hasFocus() && !skipMoveSelection) {
            chosenMove = currentPlayer->selectMove(board, window);
        }
        else if (auto* humanPlayer = dynamic_cast<HumanPlayer*>(currentPlayer)) {
            humanPlayer->resetInput();
        }

	    if (chosenMove.has_value() && !endGame) {
	        std::optional<Move> legalMove = normalizeMove(board, chosenMove.value());
            if (!legalMove.has_value()) {
                continue;
            }

	        Move move = legalMove.value();
	        std::string san = moveToSAN(board, move);
            Piece capturedPiece = capturedPieceForMove(board, move);

	        if (board.move(move)) {
	            currentPlayer->incrementTime();
	            if (auto* ai = dynamic_cast<AIPlayer*>(currentPlayer)) {
	                lastCompletedAi = ai;
	            }

	            appendCheckSuffix(san, board);
	            moveHistory.push_back({move, san, capturedPiece});
	            syncStockfishHistory();
	            restartEvaluation();
	        }
	    }

        whiteFlagged = whitePlayer->outOfTime();
        blackFlagged = blackPlayer->outOfTime();
        renderer.update(board, deltaTime, moveHistory, lastCompletedAi, whitePlayer.get(), blackPlayer.get(), whiteFlagged, blackFlagged, blackAtBottom, analyticsMode);

        for (Position position; position <= Position(7, 7); position++) {;

            Piece piece = board.getPiece(position);

            Move lastMove = moveHistory.empty() ? Move() : moveHistory.back().move;
            bool highlighted = !lastMove.isNone() && (lastMove.starting() == position || lastMove.target() == position);
            pieceDrawer.drawPiece(window, piece, position, highlighted, blackAtBottom);
        }

        MoveList moves = currentPlayer->getShownMoves();
        for (int i = 0; i < moves.count; i++) {
            moveDrawer.drawMove(window, moves[i], blackAtBottom);
        }

        renderer.draw(window, board, deltaTime);
        window.display();
    }
    return 0;
}
