//
// Created by Andreas Royset on 5/22/26.
//

#ifndef UIDRAWER_H
#define UIDRAWER_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>
#include "../AI/AIPlayer.h"
#include "../Game/Board.h"
#include "StatusIconAtlas.h"

inline std::string positionToNotation(Position position) {
    int rank = position.rank();
    int file = position.file();

    char files[8] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};

    return files[file] + std::to_string(rank+1);
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

struct MoveRecord {
    Move move;
    std::string san;
    Piece capturedPiece = EMPTY;
};

inline std::string moveToSAN(const Board& board, Move move) {
    Piece piece = board.getPiece(move.starting());

    if (move.isCastleKing())  return "O-O";
    if (move.isCastleQueen()) return "O-O-O";

    std::string san;
    char letter = pieceLetter(piece);

    if (letter != '\0') {
        san += letter;

        // Check if another piece of the same type can reach the same target
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
            if (!sameFile)
                san += char('a' + move.starting().file());
            else if (!sameRank)
                san += char('1' + move.starting().rank());
            else {
                san += char('a' + move.starting().file());
                san += char('1' + move.starting().rank());
            }
        }
    }

    if (move.isCapture()) {
        if (letter == '\0')
            san += char('a' + move.starting().file()); // pawn capture: source file
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

class uiDrawer {
    sf::Font font;
    sf::Text text{font};
    sf::Texture pieceTexture;
    sf::Sprite pieceIcon{pieceTexture};
    sf::Texture iconTexture;
    sf::Sprite statusIcon{iconTexture};
    sf::RectangleShape sidePanel;
    sf::RectangleShape divider;

    int width, height;
    float boardX = 0.0f;
    float panelX = 0.0f;
    float panelW = 0.0f;
    float pad = 16.0f;
    float evalRailW = 46.0f;

    AIPlayer* lastAi = nullptr;
    Searcher* evalSearcher = nullptr;

    float animatedWhiteScore = 0.5f;
    int fps = 0;
    std::string turnText = "White to move";
    std::string statusText = "Game in progress";
    sf::Color statusColor{152, 160, 166};
    SearchResult lastAiSearch{};
    bool hasLastAi = false;
    bool lastAiOpening = false;
    bool lastAiSearchBlackToMove = false;
    int lastAiThinkingTimeMs = 0;
    float displayedEval = 0.0f;
    bool displayedMate = false;
    int displayedDepth = 0;
    uint64_t cachedEvalHash = 0;
    float cachedStaticEval = 0.0f;
    bool cachedStaticEvalValid = false;
    std::vector<MoveRecord> history;
    bool pieceTextureLoaded = false;
    bool iconTextureLoaded = false;
    float whiteTime = 0.0f;
    float blackTime = 0.0f;
    bool whiteTimed = false;
    bool blackTimed = false;
    bool blackAtBottom = false;
    bool blackToMove = false;
    bool whiteInCheck = false;
    bool blackInCheck = false;
    bool checkmate = false;
    bool drawnGame = false;
    bool whiteFlagged = false;
    bool blackFlagged = false;
    bool canUndoMove = false;
    bool analyticsMode = false;

    sf::Color panelColor{22, 24, 27};
    sf::Color sectionColor{31, 34, 38};
    sf::Color dividerColor{53, 58, 64};
    sf::Color primaryText{238, 238, 232};
    sf::Color mutedText{152, 160, 166};
    sf::Color warningRed{221, 82, 76};
    sf::Color amber{220, 168, 72};
    sf::Color activeGreen{84, 173, 118};

    std::array<int, 16> fenCaptures{};

public:

    uiDrawer(int width, int height, float boardX, Searcher& evalSearcher)
        : boardX(boardX), evalSearcher(&evalSearcher) {
        this->width = width;
        this->height = height;
        panelX = boardX + float(height);
        panelW = float(width) - panelX;

        if (!font.openFromFile("/Users/viking/Desktop/C++ Projects/Courier Prime.ttf")) {
            std::cerr << "Error loading font\n";
            return;
        }

        sidePanel.setPosition({panelX, 0});
        sidePanel.setSize({panelW, float(height)});
        sidePanel.setFillColor(panelColor);

        pieceTextureLoaded = pieceTexture.loadFromFile("Render/ChessPieces.png");
        if (!pieceTextureLoaded) {
            std::cerr << "Error loading material piece texture\n";
        } else {
            pieceTexture.setSmooth(true);
            pieceIcon.setTexture(pieceTexture);
        }

        iconTextureLoaded = iconTexture.loadFromFile("Render/icons.png");
        if (!iconTextureLoaded) {
            std::cerr << "Error loading status icon texture\n";
        } else {
            iconTexture.setSmooth(true);
            statusIcon.setTexture(iconTexture);
        }
    }

    void draw(sf::RenderWindow& window, const Board& board, float deltaTime) {
        window.draw(sidePanel);
        drawEndStateOverlay(window, board);

        float y = panelTop();
        bool topPlayerBlack = !blackAtBottom;
        bool bottomPlayerBlack = blackAtBottom;
        float bottomPlayerY = float(height) - panelTop() - playerCardH();

        drawPlayerCard(window, board, topPlayerBlack, y, playerCardH());
        y += playerCardH() + sectionGap();

        drawControlsSection(window, y, controlsSectionH());
        y += controlsSectionH() + sectionGap();

        if (analyticsMode) {
            drawBoardEvalBar(window, displayedEval, displayedMate, deltaTime);
        }

        if (analyticsMode) {
            drawAiSection(window, y, aiSectionH());
            y += aiSectionH() + sectionGap();
        }

        float historyH = bottomPlayerY - sectionGap() - y;
        if (historyH > 64.0f) {
            drawMoveHistory(window, y, historyH);
        }

        drawPlayerCard(window, board, bottomPlayerBlack, bottomPlayerY, playerCardH());
    }

    void setAnalyticsMode(bool enabled) {
        analyticsMode = enabled;
    }

    bool analyticsButtonContains(sf::Vector2i point) const {
        return analyticsButtonBounds().contains(sf::Vector2f(float(point.x), float(point.y)));
    }

    bool undoButtonContains(sf::Vector2i point) const {
        return undoButtonBounds().contains(sf::Vector2f(float(point.x), float(point.y)));
    }

    bool flipButtonContains(sf::Vector2i point) const {
        return flipButtonBounds().contains(sf::Vector2f(float(point.x), float(point.y)));
    }

    bool copyFenButtonContains(sf::Vector2i point) const {
        return copyFenButtonBounds().contains(sf::Vector2f(float(point.x), float(point.y)));
    }

    bool pasteFenButtonContains(sf::Vector2i point) const {
        return pasteFenButtonBounds().contains(sf::Vector2f(float(point.x), float(point.y)));
    }

    void update(Board& board, float deltaTime, const std::vector<MoveRecord>& moveHistory, AIPlayer* latestAi, const Player* whitePlayer, const Player* blackPlayer, bool whiteFlagged, bool blackFlagged, bool blackAtBottom, bool analyticsMode) {
        bool whiteCheck = board.check(false);
        bool blackCheck = board.check(true);
        bool checkmate = board.checkMate();
        bool whiteCheckmated = checkmate && whiteCheck;
        bool draw = board.draw();

        fps = deltaTime > 0.0f ? int(1.0f / deltaTime) : 0;
        blackToMove = board.getPlayerTurn();
        turnText = blackToMove ? "Black to move" : "White to move";

        lastAi = latestAi;

        if (whiteFlagged) {
            statusText = "White loses on time";
            statusColor = warningRed;
        }
        else if (blackFlagged) {
            statusText = "Black loses on time";
            statusColor = warningRed;
        }
        else if (checkmate) {
            statusText = whiteCheckmated ? "Black wins by checkmate" : "White wins by checkmate";
            statusColor = warningRed;
        }
        else if (draw) {
            statusText = "Draw";
            statusColor = amber;
        }
        else if (whiteCheck) {
            statusText = "White in check";
            statusColor = warningRed;
        }
        else if (blackCheck) {
            statusText = "Black in check";
            statusColor = warningRed;
        }
        else {
            statusText = "Game in progress";
            statusColor = mutedText;
        }

        if (lastAi != nullptr) {
            lastAiSearch = lastAi->getLastSearch();
            lastAiOpening = lastAi->usedOpeningBook();
            lastAiSearchBlackToMove = lastAi->getLastSearchBlackToMove();
            lastAiThinkingTimeMs = lastAi->getLastThinkingTimeMs();
            hasLastAi = true;
        }
        else {
            hasLastAi = false;
            lastAiOpening = false;
            lastAiSearchBlackToMove = false;
            lastAiThinkingTimeMs = 0;
        }

        history = moveHistory;
        whiteTime = whitePlayer != nullptr ? whitePlayer->getTimeRemaining() : 0.0f;
        blackTime = blackPlayer != nullptr ? blackPlayer->getTimeRemaining() : 0.0f;
        whiteTimed = whitePlayer != nullptr && whitePlayer->isTimed();
        blackTimed = blackPlayer != nullptr && blackPlayer->isTimed();
        this->blackAtBottom = blackAtBottom;
        this->analyticsMode = analyticsMode;
        whiteInCheck = whiteCheck;
        blackInCheck = blackCheck;
        this->checkmate = checkmate;
        drawnGame = draw;
        this->whiteFlagged = whiteFlagged;
        this->blackFlagged = blackFlagged;
        canUndoMove = !moveHistory.empty();
        updateEvalDisplay(board);
    }

    void setFenCaptures(const std::array<int, 16>& c) { fenCaptures = c; }

private:
    
    void drawText(sf::RenderWindow& window, const std::string& value, unsigned size, float x, float y, sf::Color color, bool bold = false) {
        text.setString(value);
        text.setCharacterSize(size);
        text.setFillColor(color);
        text.setStyle(bold ? sf::Text::Bold : sf::Text::Regular);
        text.setPosition({x, y});
        window.draw(text);
    }

    void drawTextRight(sf::RenderWindow& window, const std::string& value, unsigned size, float rightX, float y, sf::Color color, bool bold = false) {
        text.setString(value);
        text.setCharacterSize(size);
        text.setFillColor(color);
        text.setStyle(bold ? sf::Text::Bold : sf::Text::Regular);

        sf::FloatRect bounds = text.getLocalBounds();
        text.setPosition({rightX - bounds.size.x - bounds.position.x, y});
        window.draw(text);
    }

    void drawTextCentered(sf::RenderWindow& window, const std::string& value, unsigned size, sf::Vector2f position, sf::Vector2f boxSize, sf::Color color, bool bold = false) {
        text.setString(value);
        text.setCharacterSize(size);
        text.setFillColor(color);
        text.setStyle(bold ? sf::Text::Bold : sf::Text::Regular);

        sf::FloatRect bounds = text.getLocalBounds();
        text.setPosition({
            position.x + (boxSize.x - bounds.size.x) * 0.5f - bounds.position.x,
            position.y + (boxSize.y - bounds.size.y) * 0.5f - bounds.position.y - 1.0f
        });

        window.draw(text);
    }

    static void drawRoundedRect(sf::RenderWindow& window, sf::Vector2f position, sf::Vector2f size, float radius, sf::Color color) {
        radius = std::max(0.0f, std::min(radius, std::min(size.x, size.y) * 0.5f));

        sf::RectangleShape center({size.x - radius * 2.0f, size.y});
        center.setPosition({position.x + radius, position.y});
        center.setFillColor(color);
        window.draw(center);

        sf::RectangleShape middle({size.x, size.y - radius * 2.0f});
        middle.setPosition({position.x, position.y + radius});
        middle.setFillColor(color);
        window.draw(middle);

        sf::CircleShape corner(radius, 32);
        corner.setFillColor(color);

        corner.setPosition(position);
        window.draw(corner);

        corner.setPosition({position.x + size.x - radius * 2.0f, position.y});
        window.draw(corner);

        corner.setPosition({position.x, position.y + size.y - radius * 2.0f});
        window.draw(corner);

        corner.setPosition({position.x + size.x - radius * 2.0f, position.y + size.y - radius * 2.0f});
        window.draw(corner);
    }

    static void drawBottomRoundedRect(sf::RenderWindow& window, sf::Vector2f position, sf::Vector2f size, float radius, sf::Color color) {
        if (size.x <= 0.0f || size.y <= 0.0f) return;

        radius = std::max(0.0f, std::min(radius, std::min(size.x, size.y) * 0.5f));

        sf::RectangleShape body({size.x, std::max(0.0f, size.y - radius)});
        body.setPosition(position);
        body.setFillColor(color);
        window.draw(body);

        if (radius <= 0.0f) return;

        sf::RectangleShape bottomCenter({size.x - radius * 2.0f, radius});
        bottomCenter.setPosition({position.x + radius, position.y + size.y - radius});
        bottomCenter.setFillColor(color);
        window.draw(bottomCenter);

        sf::CircleShape corner(radius, 32);
        corner.setFillColor(color);

        corner.setPosition({position.x, position.y + size.y - radius * 2.0f});
        window.draw(corner);

        corner.setPosition({position.x + size.x - radius * 2.0f, position.y + size.y - radius * 2.0f});
        window.draw(corner);
    }

    static void drawTopRoundedRect(sf::RenderWindow& window, sf::Vector2f position, sf::Vector2f size, float radius, sf::Color color) {
        if (size.x <= 0.0f || size.y <= 0.0f) return;

        radius = std::max(0.0f, std::min(radius, std::min(size.x, size.y) * 0.5f));

        sf::RectangleShape body({size.x, std::max(0.0f, size.y - radius)});
        body.setPosition({position.x, position.y + radius});
        body.setFillColor(color);
        window.draw(body);

        if (radius <= 0.0f) return;

        sf::RectangleShape topCenter({size.x - radius * 2.0f, radius});
        topCenter.setPosition({position.x + radius, position.y});
        topCenter.setFillColor(color);
        window.draw(topCenter);

        sf::CircleShape corner(radius, 32);
        corner.setFillColor(color);

        corner.setPosition(position);
        window.draw(corner);

        corner.setPosition({position.x + size.x - radius * 2.0f, position.y});
        window.draw(corner);
    }

    static void drawVerticalRoundedRect(sf::RenderWindow& window, sf::Vector2f position, sf::Vector2f size, float topRadius, float bottomRadius, sf::Color color) {
        if (size.x <= 0.0f || size.y <= 0.0f) return;

        float maxRadius = std::min(size.x, size.y) * 0.5f;
        topRadius = std::clamp(topRadius, 0.0f, maxRadius);
        bottomRadius = std::clamp(bottomRadius, 0.0f, maxRadius);

        if (topRadius + bottomRadius > size.y) {
            float scale = size.y / (topRadius + bottomRadius);
            topRadius *= scale;
            bottomRadius *= scale;
        }

        sf::RectangleShape center({size.x, std::max(0.0f, size.y - topRadius - bottomRadius)});
        center.setPosition({position.x, position.y + topRadius});
        center.setFillColor(color);
        window.draw(center);

        if (topRadius > 0.0f) {
            sf::RectangleShape topCenter({size.x - topRadius * 2.0f, topRadius});
            topCenter.setPosition({position.x + topRadius, position.y});
            topCenter.setFillColor(color);
            window.draw(topCenter);

            sf::CircleShape corner(topRadius, 32);
            corner.setFillColor(color);

            corner.setPosition(position);
            window.draw(corner);

            corner.setPosition({position.x + size.x - topRadius * 2.0f, position.y});
            window.draw(corner);
        } else {
            sf::RectangleShape top({size.x, 1.0f});
            top.setPosition(position);
            top.setFillColor(color);
            window.draw(top);
        }

        if (bottomRadius > 0.0f) {
            sf::RectangleShape bottomCenter({size.x - bottomRadius * 2.0f, bottomRadius});
            bottomCenter.setPosition({position.x + bottomRadius, position.y + size.y - bottomRadius});
            bottomCenter.setFillColor(color);
            window.draw(bottomCenter);

            sf::CircleShape corner(bottomRadius, 32);
            corner.setFillColor(color);

            corner.setPosition({position.x, position.y + size.y - bottomRadius * 2.0f});
            window.draw(corner);

            corner.setPosition({position.x + size.x - bottomRadius * 2.0f, position.y + size.y - bottomRadius * 2.0f});
            window.draw(corner);
        } else {
            sf::RectangleShape bottom({size.x, 1.0f});
            bottom.setPosition({position.x, position.y + size.y - 1.0f});
            bottom.setFillColor(color);
            window.draw(bottom);
        }
    }

    float evalRailX() const {
        return panelX + 10.0f;
    }

    float panelTop() const {
        return 18.0f;
    }

    float sectionGap() const {
        return 12.0f;
    }

    float playerCardH() const {
        return 88.0f;
    }

    float controlsSectionH() const {
        return 100.0f;
    }

    float aiSectionH() const {
        return 150.0f;
    }

    float controlsSectionY() const {
        return panelTop() + playerCardH() + sectionGap();
    }

    float sectionX() const {
        return analyticsMode ? panelX + evalRailW + 30.0f : panelX + 18.0f;
    }

    float sectionW() const {
        return analyticsMode ? panelW - evalRailW - 44.0f : panelW - 36.0f;
    }

    sf::FloatRect controlButtonBounds(int index) const {
        constexpr float gap = 10.0f;
        constexpr float buttonH = 28.0f;
        float x = sectionX() + pad;
        float y = controlsSectionY() + 34.0f;
        float w = sectionW() - pad * 2.0f;

        if (index < 3) {
            float buttonW = (w - gap * 2.0f) / 3.0f;
            return {{x + float(index) * (buttonW + gap), y}, {buttonW, buttonH}};
        }

        float buttonW = (w - gap) / 2.0f;
        return {{x + float(index - 3) * (buttonW + gap), y + 34.0f}, {buttonW, buttonH}};
    }

    sf::FloatRect analyticsButtonBounds() const {
        return controlButtonBounds(2);
    }

    sf::FloatRect undoButtonBounds() const {
        return controlButtonBounds(1);
    }

    sf::FloatRect flipButtonBounds() const {
        return controlButtonBounds(0);
    }

    sf::FloatRect copyFenButtonBounds() const {
        return controlButtonBounds(3);
    }

    sf::FloatRect pasteFenButtonBounds() const {
        return controlButtonBounds(4);
    }

    void drawSection(sf::RenderWindow& window, float y, float h) {
        drawRoundedRect(window, {sectionX(), y}, {sectionW(), h}, 8.0f, sectionColor);
    }

    void drawDivider(sf::RenderWindow& window, float x, float y, float w) {
        divider.setPosition({x, y});
        divider.setSize({w, 1.0f});
        divider.setFillColor(dividerColor);
        window.draw(divider);
    }

    void drawPlayerCard(sf::RenderWindow& window, const Board& board, bool black, float y, float h) {
        auto captures = buildCapturedCounts();
        int whiteMaterial = 0;
        int blackMaterial = 0;

        for (Position position; position <= Position(7, 7); position++) {
            Piece piece = board.getPiece(position);
            int value = materialValue(piece);

            if (piece == EMPTY) continue;
            if (piece >> 3) {
                blackMaterial += value;
            } else {
                whiteMaterial += value;
            }
        }

        int materialDiff = whiteMaterial - blackMaterial;

        drawSection(window, y, h);
        drawPlayerRow(window, black, captures, black ? (materialDiff < 0 ? -materialDiff : 0) : (materialDiff > 0 ? materialDiff : 0), y + 7.0f);
    }

    void drawPlayerRow(sf::RenderWindow& window, bool black, const std::array<int, 16>& captures, int materialPlus, float y) {
        float x = sectionX() + pad;
        float w = sectionW() - pad * 2.0f;
        constexpr float rowH = 74.0f;
        bool active = black == blackToMove;
        sf::Color rowFill = active ? sf::Color(43, 51, 45) : sf::Color(27, 30, 34);
        sf::Color lineColor = active ? activeGreen : dividerColor;
        float seconds = black ? blackTime : whiteTime;
        bool timed = black ? blackTimed : whiteTimed;

        sf::RectangleShape row({w, rowH});
        row.setPosition({x, y});
        row.setFillColor(rowFill);
        window.draw(row);

        sf::RectangleShape activeLine({4.0f, rowH});
        activeLine.setPosition({x, y});
        activeLine.setFillColor(lineColor);
        window.draw(activeLine);

        drawText(window, black ? "BLACK" : "WHITE", 24, x + 14.0f, y + 8.0f, active ? primaryText : mutedText, true);
        drawTextRight(window, timed ? formatClock(seconds) : "--:--", 54, x + w - 14.0f, y - 1.0f, timed ? clockColor(seconds) : primaryText, true);
        drawCapturedMaterial(window, black, captures, materialPlus, x + 14.0f, y + 49.0f);
    }

    void drawControlsSection(sf::RenderWindow& window, float y, float h) {
        float x = sectionX() + pad;

        drawSection(window, y, h);
        drawText(window, "CONTROLS", 13, x, y + 14.0f, mutedText, true);
        drawButton(window, flipButtonBounds(), "Flip Board", true);
        drawButton(window, undoButtonBounds(), "Undo", canUndoMove);
        drawButton(window, analyticsButtonBounds(), analyticsMode ? "Analytics On" : "Analytics Off", true, analyticsMode);
        drawButton(window, copyFenButtonBounds(), "Copy FEN", true);
        drawButton(window, pasteFenButtonBounds(), "Paste FEN", true);
    }

    void drawMaterialSection(sf::RenderWindow& window, const Board& board, float y, float h) {
        float x = sectionX() + pad;
        auto captures = buildCapturedCounts();
        int whiteMaterial = 0;
        int blackMaterial = 0;

        for (Position position; position <= Position(7, 7); position++) {
            Piece piece = board.getPiece(position);
            int value = materialValue(piece);

            if (piece == EMPTY) continue;
            if (piece >> 3) {
                blackMaterial += value;
            } else {
                whiteMaterial += value;
            }
        }

        drawSection(window, y, h);
        drawText(window, "MATERIAL", 13, x, y + 14.0f, mutedText, true);
        int materialDiff = whiteMaterial - blackMaterial;
        if (blackAtBottom) {
            drawCapturedRow(window, "WHITE", captures, true, y + 38.0f, materialDiff > 0 ? materialDiff : 0);
            drawCapturedRow(window, "BLACK", captures, false, y + 82.0f, materialDiff < 0 ? -materialDiff : 0);
        } else {
            drawCapturedRow(window, "BLACK", captures, false, y + 38.0f, materialDiff < 0 ? -materialDiff : 0);
            drawCapturedRow(window, "WHITE", captures, true, y + 82.0f, materialDiff > 0 ? materialDiff : 0);
        }
    }

    void drawAiSection(sf::RenderWindow& window, float y, float h) {
        float x = sectionX() + pad;

        drawSection(window, y, h);
        drawText(window, "LAST AI SEARCH", 13, x, y + 14.0f, mutedText, true);

        if (!hasLastAi || !lastAiSearch.bestMove.has_value()) {
            drawText(window, "No completed search yet", 16, x, y + 48.0f, mutedText);
            return;
        }

        if (lastAiOpening) {
            drawStatRow(window, "Mode", "Opening", y + 46.0f);
            return;
        }

        float aiEval = lastAiSearchBlackToMove ? -lastAiSearch.eval : lastAiSearch.eval;

        drawStatRow(window, "Depth", std::to_string(lastAiSearch.depth), y + 42.0f);
        drawStatRow(window, "Eval", formatEval(aiEval, lastAiSearch.mate), y + 60.0f);
        drawStatRow(window, "Evals", std::to_string(lastAiSearch.positionsEvaluated), y + 78.0f);
        drawStatRow(window, "Nodes/s", std::to_string(lastAiSearch.nodesPerSecond), y + 96.0f);
        drawStatRow(window, "TT", std::to_string(lastAiSearch.ttLookups), y + 114.0f);
        drawStatRow(window, "Thinking time", std::to_string(lastAiThinkingTimeMs) + " ms", y + 132.0f);
    }

    void drawStatRow(sf::RenderWindow& window, const std::string& label, const std::string& value, float y) {
        float x = sectionX() + pad;
        drawText(window, label, 15, x, y, mutedText);
        drawText(window, value, 15, x + 126.0f, y, primaryText, true);
    }

    void drawButton(sf::RenderWindow& window, sf::FloatRect bounds, const std::string& label, bool enabled, bool active = false) {
        sf::Color border = enabled ? (active ? amber : sf::Color(77, 84, 91)) : sf::Color(43, 47, 52);
        sf::Color fill = enabled ? (active ? sf::Color(68, 57, 34) : sf::Color(47, 52, 58)) : sf::Color(34, 37, 41);
        sf::Color labelColor = enabled ? primaryText : sf::Color(104, 111, 118);

        drawRoundedRect(window, bounds.position, bounds.size, 6.0f, border);
        drawRoundedRect(
            window,
            {bounds.position.x + 1.0f, bounds.position.y + 1.0f},
            {bounds.size.x - 2.0f, bounds.size.y - 2.0f},
            5.0f,
            fill
        );
        drawTextCentered(window, label, 14, bounds.position, bounds.size, labelColor, true);
    }

    void drawMoveHistory(sf::RenderWindow& window, float y, float h) {
        float x = sectionX() + pad;
        float rowH = 22.0f;

        drawSection(window, y, h);
        drawText(window, "MOVE HISTORY", 13, x, y + 14.0f, mutedText, true);
        drawText(window, "#", 13, x, y + 42.0f, mutedText, true);
        drawText(window, "WHITE", 13, x + 54.0f, y + 42.0f, mutedText, true);
        drawText(window, "BLACK", 13, x + sectionW() * 0.48f, y + 42.0f, mutedText, true);
        drawDivider(window, x, y + 64.0f, sectionW() - pad * 2.0f);

        if (history.empty()) {
            drawText(window, "No moves yet", 16, x, y + 82.0f, mutedText);
            return;
        }

        int maxRows = std::max(1, int((h - 82.0f) / rowH));
        int lastRow = int(history.size() - 1) / 2;
        int firstRow = std::max(0, lastRow - maxRows + 1);
        float rowY = y + 76.0f;
        for (int row = firstRow; row <= lastRow; row++) {
            int whiteMove = row * 2;
            int blackMove = whiteMove + 1;

            drawText(window, std::to_string(row + 1), 15, x, rowY, mutedText);
            drawMoveCell(window, history[whiteMove].san, x + 54.0f, rowY, whiteMove == int(history.size()) - 1);

            if (blackMove < history.size()) {
                drawMoveCell(window, history[blackMove].san, x + sectionW() * 0.48f, rowY, blackMove == int(history.size()) - 1);
            }

            rowY += rowH;
        }
    }

    void drawMoveCell(sf::RenderWindow& window, const std::string& san, float x, float y, bool latest) {
        if (latest) {
            drawRoundedRect(window, {x - 7.0f, y - 2.0f}, {110.0f, 21.0f}, 4.0f, sf::Color(61, 67, 73));
        }

        drawText(window, san, 16, x, y, latest ? primaryText : sf::Color(214, 216, 210), latest);
    }

    void drawEndStateOverlay(sf::RenderWindow& window, const Board& board) {
        if (!checkmate && !drawnGame && !whiteFlagged && !blackFlagged) return;

        Position whiteKing = findKing(board, false);
        Position blackKing = findKing(board, true);

        if (whiteFlagged || blackFlagged) {
            if (whiteFlagged) {
                if (!blackKing.isNone()) drawStatusIcon(window, StatusIcon::Crown, blackKing);
                if (!whiteKing.isNone()) drawStatusIcon(window, StatusIcon::WhiteClock, whiteKing);
            }
            if (blackFlagged) {
                if (!whiteKing.isNone()) drawStatusIcon(window, StatusIcon::Crown, whiteKing);
                if (!blackKing.isNone()) drawStatusIcon(window, StatusIcon::BlackClock, blackKing);
            }
            return;
        }

        if (checkmate) {
            if (whiteInCheck && !blackKing.isNone()) {
                drawStatusIcon(window, StatusIcon::Crown, blackKing);
                drawStatusIcon(window, StatusIcon::WhiteCheckmate, whiteKing);
            }
            else if (blackInCheck && !whiteKing.isNone()) {
                drawStatusIcon(window, StatusIcon::Crown, whiteKing);
                drawStatusIcon(window, StatusIcon::BlackCheckmate, blackKing);
            }
            return;
        }

        if (!whiteKing.isNone()) drawStatusIcon(window, StatusIcon::Draw, whiteKing);
        if (!blackKing.isNone()) drawStatusIcon(window, StatusIcon::Draw, blackKing);
    }

    Position findKing(const Board& board, bool black) const {
        Piece king = black ? BLACK_KING : WHITE_KING;
        for (Position position; position <= Position(7, 7); position++) {
            if (board.getPiece(position) == king) return position;
        }

        Position none;
        none.setNone();
        return none;
    }

    sf::Vector2f boardSquareOrigin(Position position) const {
        float tileSize = float(height) / 8.0f;
        int displayFile = blackAtBottom ? 7 - position.file() : position.file();
        int displayRank = blackAtBottom ? position.rank() : 7 - position.rank();

        return {boardX + float(displayFile) * tileSize, float(displayRank) * tileSize};
    }

    void drawStatusIcon(sf::RenderWindow& window, StatusIcon icon, Position kingPosition) {
        if (!iconTextureLoaded || kingPosition.isNone()) return;

        float tileSize = float(height) / 8.0f;
        sf::Vector2f origin = boardSquareOrigin(kingPosition);
        sf::IntRect sourceRect = statusIconRect(icon);

        float iconSize = tileSize * 0.34f;
        float margin = tileSize * 0.06f;
        sf::Vector2f iconPos{
            origin.x + tileSize - iconSize - margin,
            origin.y + margin
        };

        statusIcon.setTextureRect(sourceRect);
        statusIcon.setScale({iconSize / float(sourceRect.size.x), iconSize / float(sourceRect.size.y)});
        statusIcon.setPosition(iconPos);
        statusIcon.setColor(sf::Color::White);
        window.draw(statusIcon);
    }

    std::array<int, 16> buildCapturedCounts() const {
        std::array<int, 16> counts = fenCaptures;
        for (const MoveRecord& record : history) {
            if (record.capturedPiece != EMPTY) {
                counts[record.capturedPiece]++;
            }
        }
        return counts;
    }

    void drawCapturedMaterial(sf::RenderWindow& window, bool black, const std::array<int, 16>& captures, int materialPlus, float x, float y) {
        const std::array<Piece, 5> pieces = black
            ? std::array<Piece, 5>{WHITE_QUEEN, WHITE_ROOK, WHITE_BISHOP, WHITE_KNIGHT, WHITE_PAWN}
            : std::array<Piece, 5>{BLACK_QUEEN, BLACK_ROOK, BLACK_BISHOP, BLACK_KNIGHT, BLACK_PAWN};

        float iconX = x;
        if (materialPlus > 0) {
            drawText(window, "+" + std::to_string(materialPlus), 15, iconX, y + 2.0f, primaryText, true);
            iconX += 34.0f;
        }

        constexpr float iconSize = 24.0f;
        constexpr float stackStep = 8.0f;
        constexpr float groupGap = 8.0f;

        for (Piece piece : pieces) {
            int count = captures[piece];
            if (count <= 0) continue;

            for (int i = 0; i < count; i++) {
                drawPieceIcon(window, piece, iconX + float(i) * stackStep, y, iconSize);
            }

            iconX += iconSize + float(count - 1) * stackStep + groupGap;
        }
    }

    void drawCapturedRow(sf::RenderWindow& window, const std::string& label, const std::array<int, 16>& captures, bool whiteCaptured, float y, int materialPlus) {
        float x = sectionX() + pad;
        const std::array<Piece, 5> pieces = whiteCaptured
            ? std::array<Piece, 5>{BLACK_QUEEN, BLACK_ROOK, BLACK_BISHOP, BLACK_KNIGHT, BLACK_PAWN}
            : std::array<Piece, 5>{WHITE_QUEEN, WHITE_ROOK, WHITE_BISHOP, WHITE_KNIGHT, WHITE_PAWN};
        bool hasCaptures = false;
        bool active = (label == "BLACK") == blackToMove;

        if (active) {
            sf::RectangleShape activeLine({4.0f, 32.0f});
            activeLine.setPosition({x, y - 8.0f});
            activeLine.setFillColor(activeGreen);
            window.draw(activeLine);
        }

        drawText(window, label, 13, x + (active ? 12.0f : 0.0f), y, active ? primaryText : mutedText, true);
        if (materialPlus > 0) {
            drawText(window, "+" + std::to_string(materialPlus), 16, x + 58.0f, y - 2.0f, primaryText, true);
        }

        float iconSize = 28.0f;
        float stackStep = 10.0f;
        float groupGap = 9.0f;
        float iconX = x + 96.0f;
        float iconY = y - 7.0f;

        for (Piece piece : pieces) {
            int count = captures[piece];
            if (count <= 0) continue;

            hasCaptures = true;
            for (int i = 0; i < count; i++) {
                drawPieceIcon(window, piece, iconX + float(i) * stackStep, iconY, iconSize);
            }

            iconX += iconSize + float(count - 1) * stackStep + groupGap;
        }

        if (!hasCaptures) {
            drawText(window, "", 15, x, y + 22.0f, mutedText);
        }
    }

    void drawPieceIcon(sf::RenderWindow& window, Piece piece, float x, float y, float size) {
        if (!pieceTextureLoaded || piece == EMPTY) return;

        int sheetX = 0;
        int sheetY = 0;
        getSpriteSheetCoords(piece, sheetX, sheetY);

        sf::Vector2u textureSize = pieceTexture.getSize();
        int cellW = int(textureSize.x) / 6;
        int cellH = int(textureSize.y) / 2;

        pieceIcon.setTextureRect(sf::IntRect({sheetX * cellW, sheetY * cellH}, {cellW, cellH}));
        pieceIcon.setScale({size / float(cellW), size / float(cellH)});
        pieceIcon.setPosition({x, y});
        pieceIcon.setColor(sf::Color::White);
        window.draw(pieceIcon);
    }

    static int materialValue(Piece piece) {
        switch (piece) {
            case WHITE_PAWN:
            case BLACK_PAWN:
                return 1;
            case WHITE_KNIGHT:
            case BLACK_KNIGHT:
            case WHITE_BISHOP:
            case BLACK_BISHOP:
                return 3;
            case WHITE_ROOK:
            case BLACK_ROOK:
                return 5;
            case WHITE_QUEEN:
            case BLACK_QUEEN:
                return 9;
            default:
                return 0;
        }
    }

    static void getSpriteSheetCoords(Piece piece, int& x, int& y) {
        switch (piece) {
            case BLACK_KING:
                x = 0;
                y = 1;
                break;
            case BLACK_QUEEN:
                x = 1;
                y = 1;
                break;
            case BLACK_BISHOP:
                x = 2;
                y = 1;
                break;
            case BLACK_KNIGHT:
                x = 3;
                y = 1;
                break;
            case BLACK_ROOK:
                x = 4;
                y = 1;
                break;
            case BLACK_PAWN:
                x = 5;
                y = 1;
                break;
            case WHITE_KING:
                x = 0;
                y = 0;
                break;
            case WHITE_QUEEN:
                x = 1;
                y = 0;
                break;
            case WHITE_BISHOP:
                x = 2;
                y = 0;
                break;
            case WHITE_KNIGHT:
                x = 3;
                y = 0;
                break;
            case WHITE_ROOK:
                x = 4;
                y = 0;
                break;
            case WHITE_PAWN:
                x = 5;
                y = 0;
                break;
            default:
                x = 0;
                y = 0;
                break;
        }
    }

    void updateEvalDisplay(const Board& board) {
        if (!analyticsMode) return;

        uint64_t hash = board.getHash();
        if (!cachedStaticEvalValid || cachedEvalHash != hash) {
            cachedStaticEval = Evaluator::evaluate(board);
            cachedEvalHash = hash;
            cachedStaticEvalValid = true;
        }

        displayedEval = board.getPlayerTurn() ? -cachedStaticEval : cachedStaticEval;
        displayedMate = false;
        displayedDepth = 0;

        if (evalSearcher == nullptr) return;

        SearchResult search = evalSearcher->getResult();
        if (!search.bestMove.has_value()) return;

        displayedEval = board.getPlayerTurn() ? -search.eval : search.eval;
        displayedMate = search.mate;
        displayedDepth = search.depth;
    }

    void drawBoardEvalBar(sf::RenderWindow& window, float eval, bool mate, float deltaTime) {
        float x = evalRailX();
        float y = 12.0f;
        float w = evalRailW;
        float h = float(height) - 24.0f;
        float inset = 2.0f;
        float ix = x + inset;
        float iy = y + inset;
        float iw = w - inset * 2.0f;
        float ih = h - inset * 2.0f;

        float targetWhiteScore;

        if (mate) {
            targetWhiteScore = eval > 0 ? 1.0f : 0.0f;
        } else {
            targetWhiteScore = 0.5f + 0.5f * std::tanh(eval / 4.0f);
        }

        float speed = 8.0f;
        animatedWhiteScore += (targetWhiteScore - animatedWhiteScore)
                             * std::clamp(deltaTime * speed, 0.0f, 1.0f);

        float innerWhiteHeight = ih * animatedWhiteScore;

        drawRoundedRect(window, {x, y}, {w, h}, w * 0.5f, dividerColor);
        drawRoundedRect(window, {ix, iy}, {iw, ih}, iw * 0.5f, sf::Color(18, 19, 21));

        if (innerWhiteHeight > 1.0f) {
            sf::Color whiteFill{230, 230, 230};
            float capRadius = iw * 0.5f;
            float splitRadius = std::clamp(innerWhiteHeight - (ih - capRadius), 0.0f, capRadius);
            if (innerWhiteHeight >= ih - 1.0f) {
                drawRoundedRect(window, {ix, iy}, {iw, ih}, iw * 0.5f, whiteFill);
            }
            else if (blackAtBottom) {
                drawVerticalRoundedRect(window, {ix, iy}, {iw, innerWhiteHeight}, capRadius, splitRadius, whiteFill);
            }
            else {
                drawVerticalRoundedRect(window, {ix, iy + ih - innerWhiteHeight}, {iw, innerWhiteHeight}, splitRadius, capRadius, whiteFill);
            }
        }

        drawEvalValue(window, formatAbsEval(eval, mate), eval, {ix, iy}, {iw, ih}, innerWhiteHeight);
        drawDepthMarker(window, displayedDepth, eval, {ix, iy}, {iw, ih}, innerWhiteHeight);

    }

    void drawEvalValue(sf::RenderWindow& window, const std::string& label, float eval, sf::Vector2f position, sf::Vector2f size, float whiteHeight) {
        bool whiteWinning = eval >= 0.0f;
        constexpr float labelH = 22.0f;
        constexpr float labelGap = 2.0f;
        float boundaryY = blackAtBottom
            ? position.y + whiteHeight
            : position.y + size.y - whiteHeight;
        boundaryY = std::clamp(boundaryY, position.y + 1.0f, position.y + size.y - 2.0f);
        bool whiteOnTop = blackAtBottom;
        bool winningSideOnTop = whiteWinning == whiteOnTop;
        float labelY = winningSideOnTop ? boundaryY - labelH - labelGap : boundaryY + labelGap;
        sf::Color color = whiteWinning ? sf::Color(18, 19, 21) : primaryText;

        sf::RectangleShape separator({size.x, 1.0f});
        separator.setPosition({position.x, boundaryY});
        separator.setFillColor(sf::Color(118, 124, 130, 160));
        window.draw(separator);

        drawTextCentered(
            window,
            label,
            13,
            {position.x, std::clamp(labelY, position.y + 8.0f, position.y + size.y - labelH - 8.0f)},
            {size.x, labelH},
            color,
            true
        );
    }

    void drawDepthMarker(sf::RenderWindow& window, int depth, float eval, sf::Vector2f position, sf::Vector2f size, float whiteHeight) {
        if (depth <= 0) return;

        std::string label = "d" + std::to_string(depth);
        constexpr float markerH = 18.0f;
        constexpr float markerGap = 8.0f;
        bool whiteWinning = eval >= 0.0f;
        bool whiteOnTop = blackAtBottom;
        bool winningSideOnTop = whiteWinning == whiteOnTop;
        float boundaryY = blackAtBottom
            ? position.y + whiteHeight
            : position.y + size.y - whiteHeight;
        boundaryY = std::clamp(boundaryY, position.y + 1.0f, position.y + size.y - 2.0f);

        float markerY = winningSideOnTop
            ? position.y + markerGap
            : position.y + size.y - markerH - markerGap;
        markerY = winningSideOnTop
            ? std::min(markerY, boundaryY - markerH - markerGap)
            : std::max(markerY, boundaryY + markerGap);
        markerY = std::clamp(markerY, position.y + markerGap, position.y + size.y - markerH - markerGap);

        sf::Vector2f markerPos{position.x, markerY};
        sf::Color color = whiteWinning ? sf::Color(18, 19, 21) : primaryText;

        drawTextCentered(window, label, 10, markerPos, {size.x, markerH}, color, true);
    }

    static std::string formatEval(float eval, bool mate) {
        std::ostringstream ss;
        if (mate) {
            ss << 'M' << int(std::abs(eval));
            return ss.str();
        }

        if (eval > 0.0f) ss << '+';
        ss << std::fixed << std::setprecision(2) << eval;
        return ss.str();
    }

    static std::string formatAbsEval(float eval, bool mate) {
        std::ostringstream ss;
        if (mate) {
            ss << 'M' << int(std::abs(eval));
            return ss.str();
        }

        ss << std::fixed << std::setprecision(2) << std::abs(eval);
        return ss.str();
    }

    static std::string formatClock(float seconds) {
        if (seconds < 60.0f) {
            int centiseconds = std::max(0, int(std::ceil(seconds * 100.0f)));
            centiseconds = std::min(5999, centiseconds);

            std::ostringstream ss;
            ss << centiseconds / 100 << '.' << std::setw(2) << std::setfill('0') << centiseconds % 100;
            return ss.str();
        }

        int totalSeconds = std::max(0, int(std::ceil(seconds)));
        int minutes = totalSeconds / 60;
        int secs = totalSeconds % 60;

        std::ostringstream ss;
        ss << minutes << ':' << std::setw(2) << std::setfill('0') << secs;
        return ss.str();
    }

    sf::Color clockColor(float seconds) const {
        return seconds < 20.0f ? warningRed : primaryText;
    }
};

#endif //UIDRAWER_H
