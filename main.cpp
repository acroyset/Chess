#include <iostream>
#include <sstream>

#include "AI/AIPlayer.h"
#include "HumanPlayer.h"
#include "Render/MoveDrawer.h"
#include "Render/PieceDrawer.h"
#include "Player.h"
#include "Render/uiDrawer.h"

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

    Searcher searcher;
    uiDrawer renderer(width, height, searcher);

    Board board;
    PieceDrawer pieceDrawer(tileSize);
    MoveDrawer moveDrawer(tileSize);

    std::unique_ptr<Player> whitePlayer = std::make_unique<HumanPlayer>(tileSize, 15.0f * 60.0f, 10.0f);
    std::unique_ptr<Player> blackPlayer = std::make_unique<AIPlayer>(&searcher, 1.0f, 10.0f * 60.0f, 10.0f);


    bool whiteHuman = dynamic_cast<HumanPlayer*>(whitePlayer.get()) != nullptr;
    bool blackHuman = dynamic_cast<HumanPlayer*>(blackPlayer.get()) != nullptr;
    bool whiteAi = dynamic_cast<AIPlayer*>(whitePlayer.get()) != nullptr;
    bool blackAi = dynamic_cast<AIPlayer*>(blackPlayer.get()) != nullptr;
    bool blackAtBottom = blackHuman && whiteAi && !whiteHuman && !blackAi;

    if (auto* humanPlayer = dynamic_cast<HumanPlayer*>(whitePlayer.get())) {
        humanPlayer->setBlackAtBottom(blackAtBottom);
    }
    if (auto* humanPlayer = dynamic_cast<HumanPlayer*>(blackPlayer.get())) {
        humanPlayer->setBlackAtBottom(blackAtBottom);
    }

    std::vector<MoveRecord> moveHistory;

    searcher.startSearch(board);
    sf::Clock clock;
    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    window.close();
                }
            }
        }

        float deltaTime = clock.restart().asSeconds();

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

        renderer.update(board, deltaTime, moveHistory, currentPlayer, whitePlayer.get(), blackPlayer.get(), whiteFlagged, blackFlagged, blackAtBottom);

	    std::optional<Move> chosenMove = std::nullopt;
        if (!endGame && window.hasFocus()) {
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

	            appendCheckSuffix(san, board);
	            moveHistory.push_back({move, san, capturedPiece});
	            searcher.endSearch();
	            searcher.startSearch(board);
	        }
	    }

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
