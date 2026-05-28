//
// Created by Codex on 5/27/26.
//

#include "GameRunners.h"

#include <chrono>
#include <iostream>
#include <optional>
#include <thread>

#include <SFML/Graphics.hpp>

#include "../HumanPlayer.h"
#include "FenSaveLoader.h"
#include "../Render/MoveDrawer.h"
#include "../Render/PieceDrawer.h"
#include "../Render/uiDrawer.h"

constexpr float BOARD_X = 0.0f;

sf::RenderWindow createWindow() {
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antiAliasingLevel = 8;

    sf::RenderWindow window(
        sf::VideoMode::getDesktopMode(),
        "Fullscreen Window",
        sf::Style::Default,
        sf::State::Fullscreen,
        settings
    );
    window.setFramerateLimit(60);

    const sf::ContextSettings actualSettings = window.getSettings();
    if (actualSettings.antiAliasingLevel < settings.antiAliasingLevel) {
        std::cerr << "Requested " << settings.antiAliasingLevel
                  << "x anti-aliasing, got "
                  << actualSettings.antiAliasingLevel << "x\n";
    }

    return window;
}

void setHumanWindow(GameManager& game, sf::RenderWindow& window, bool blackAtBottom) {
    if (auto* humanPlayer = dynamic_cast<HumanPlayer*>(game.getWhitePlayer())) {
        humanPlayer->setBlackAtBottom(blackAtBottom);
        humanPlayer->setInputWindow(&window);
    }
    if (auto* humanPlayer = dynamic_cast<HumanPlayer*>(game.getBlackPlayer())) {
        humanPlayer->setBlackAtBottom(blackAtBottom);
        humanPlayer->setInputWindow(&window);
    }
}

void resetCurrentHumanInput(GameManager& game) {
    if (auto* humanPlayer = dynamic_cast<HumanPlayer*>(game.currentPlayer())) {
        humanPlayer->resetInput();
    }
}

void resetAllHumanInput(GameManager& game) {
    if (auto* humanPlayer = dynamic_cast<HumanPlayer*>(game.getWhitePlayer())) {
        humanPlayer->resetInput();
    }
    if (auto* humanPlayer = dynamic_cast<HumanPlayer*>(game.getBlackPlayer())) {
        humanPlayer->resetInput();
    }
}

void restartEvaluation(Searcher& searcher, GameManager& game) {
    searcher.endSearch();
    searcher.startSearch(game.getBoard());
}

bool loadFenFromClipboard(GameManager& game, uiDrawer& renderer, Searcher& searcher) {
    std::string fen = pasteFromClipboard();
    if (!game.loadFen(fen)) {
        std::cerr << "Invalid FEN\n";
        return false;
    }

    auto fenCaptures = inferCapturedFromFEN(game.getBoard());
    renderer.setFenCaptures(fenCaptures);
    restartEvaluation(searcher, game);
    std::cerr << "FEN loaded OK\n";
    return true;
}

void copyCurrentFen(GameManager& game) {
    std::string fen = boardToFEN(game.getBoard());
    copyToClipboard(fen);
    std::cerr << "FEN: " << fen << "\n";
}

Result gameResult(GameManager& game) {
    if (game.isWhiteFlagged()) return BlackWin;
    if (game.isBlackFlagged()) return WhiteWin;
    if (game.getBoard().draw()) return Draw;
    if (game.getBoard().checkMate()) {
        return game.getBoard().getPlayerTurn() ? WhiteWin : BlackWin;
    }

    return NoResult;
}


Result ChessApp::runHeadlessGame(GameManager& game, Searcher& searcher) {
    auto lastTick = std::chrono::steady_clock::now();
    while (!game.isGameOver() && int(game.getMoveHistory().size())) {
        auto now = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(now - lastTick).count();
        lastTick = now;

        game.runGame(deltaTime);
    }

    searcher.endSearch();
    return gameResult(game);
}

Result ChessApp::runRenderedGame(GameManager& game, Searcher& searcher) {
    sf::RenderWindow window = createWindow();

    int width = int(window.getSize().x);
    int height = int(window.getSize().y);
    float tileSize = float(height) / 8.0f;

    uiDrawer renderer(width, height, BOARD_X, searcher);
    PieceDrawer pieceDrawer(tileSize, BOARD_X);
    MoveDrawer moveDrawer(tileSize, BOARD_X);

    bool blackAtBottom = false;
    bool analyticsMode = true;
    setHumanWindow(game, window, blackAtBottom);
    restartEvaluation(searcher, game);

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
                if (keyPressed->code == sf::Keyboard::Key::F) {
                    pasteFenRequested = true;
                }
                if (keyPressed->code == sf::Keyboard::Key::C) {
                    copyFenRequested = true;
                }
                if (keyPressed->code == sf::Keyboard::Key::B) {
                    flipBoardRequested = true;
                }
                if (keyPressed->code == sf::Keyboard::Key::P) {
                    copyToClipboard(exportPGN(game.getMoveHistory()));
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
            setHumanWindow(game, window, blackAtBottom);
            resetAllHumanInput(game);
        }

        if (copyFenRequested) {
            copyCurrentFen(game);
        }

        if (pasteFenRequested) {
            loadFenFromClipboard(game, renderer, searcher);
        }

        bool skipMoveSelection = uiControlClicked || flipBoardRequested || copyFenRequested || pasteFenRequested;
        if (undoRequested && game.undoMove()) {
            skipMoveSelection = true;
            restartEvaluation(searcher, game);
        }

        window.clear();

        bool canSelectMove = window.hasFocus() && !skipMoveSelection;
        if (!canSelectMove) {
            resetCurrentHumanInput(game);
        }

        GameAdvanceResult advance = game.runGame(deltaTime, canSelectMove);
        if (advance.moveApplied) {
            restartEvaluation(searcher, game);
        }

        renderer.update(
            game.getBoard(),
            deltaTime,
            game.getMoveHistory(),
            game.getLastCompletedAi(),
            game.getWhitePlayer(),
            game.getBlackPlayer(),
            game.isWhiteFlagged(),
            game.isBlackFlagged(),
            blackAtBottom,
            analyticsMode
        );

        for (Position position; position <= Position(7, 7); position++) {
            Piece piece = game.getBoard().getPiece(position);
            Move lastMove = game.getLastMove();
            bool highlighted = !lastMove.isNone() && (lastMove.starting() == position || lastMove.target() == position);
            pieceDrawer.drawPiece(window, piece, position, highlighted, blackAtBottom);
        }

        MoveList moves = game.getShownMoves();
        for (int i = 0; i < moves.count; i++) {
            moveDrawer.drawMove(window, moves[i], blackAtBottom);
        }

        renderer.draw(window, game.getBoard(), deltaTime);
        window.display();
    }

    searcher.endSearch();
    return gameResult(game);
}
