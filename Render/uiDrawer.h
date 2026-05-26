//
// Created by Andreas Royset on 5/22/26.
//

#ifndef UIDRAWER_H
#define UIDRAWER_H

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>
#include "../AI/AIPlayer.h"
#include "../Board.h"

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

    if (move.isCastleKing()) {
        return "O-O";
    }

    if (move.isCastleQueen()) {
        return "O-O-O";
    }

    std::string san;

    char letter = pieceLetter(piece);

    if (letter != '\0') {
        san += letter;
    }

    if (move.isCapture()) {
        if (letter == '\0') {
            san += char('a' + move.starting().file());
        }

        san += 'x';
    }

    san += positionToNotation(move.target());

    if ((piece == WHITE_PAWN && move.target().rank() == 7) || (piece == BLACK_PAWN && move.target().rank() == 0)) {
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
    sf::RectangleShape sidePanel;
    sf::RectangleShape divider;

    int width, height;
    float panelX = 0.0f;
    float panelW = 0.0f;
    float pad = 16.0f;
    float evalRailW = 22.0f;

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
    std::vector<MoveRecord> history;
    bool pieceTextureLoaded = false;
    float whiteTime = 0.0f;
    float blackTime = 0.0f;
    bool whiteTimed = false;
    bool blackTimed = false;
    bool blackAtBottom = false;
    bool canUndoMove = false;
    bool analyticsMode = false;

    sf::Color panelColor{22, 24, 27};
    sf::Color sectionColor{31, 34, 38};
    sf::Color dividerColor{53, 58, 64};
    sf::Color primaryText{238, 238, 232};
    sf::Color mutedText{152, 160, 166};
    sf::Color warningRed{221, 82, 76};
    sf::Color amber{220, 168, 72};

    std::array<int, 16> fenCaptures{};

public:

    uiDrawer(int width, int height, Searcher& evalSearcher)
        : evalSearcher(&evalSearcher) {
        this->width = width;
        this->height = height;
        panelX = float(height);
        panelW = float(width - height);

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
    }

    void draw(sf::RenderWindow& window, const Board& board, float deltaTime) {
        window.draw(sidePanel);
        drawGameSection(window);

        if (analyticsMode) {
            float currentEval = evaluateBoard(board);
            if (board.getPlayerTurn()) {
                currentEval = -currentEval;
            }
            bool currentMate = false;
            int evalDepth = 0;

            if (evalSearcher != nullptr) {
                SearchResult search = evalSearcher->getResult();

                if (search.bestMove.has_value()) {
                    currentEval = search.eval;
                    currentMate = search.mate;
                    evalDepth = search.depth;

                    if (board.getPlayerTurn()) {
                        currentEval = -currentEval;
                    }
                }
            }

            drawBoardEvalBar(window, currentEval, currentMate, deltaTime);
            drawEvalSection(window, currentEval, currentMate, evalDepth);
        }

        drawMaterialSection(window, board);
        if (analyticsMode) {
            drawAiSection(window);
        }
        drawMoveHistory(window);
    }

    void setAnalyticsMode(bool enabled) {
        analyticsMode = enabled;
    }

    bool analyticsButtonContains(sf::Vector2i point) const {
        return analyticsButtonBounds().contains(sf::Vector2f(float(point.x), float(point.y)));
    }

    bool undoButtonContains(sf::Vector2i point) const {
        if (!analyticsMode) return false;
        return undoButtonBounds().contains(sf::Vector2f(float(point.x), float(point.y)));
    }

    void update(Board& board, float deltaTime, const std::vector<MoveRecord>& moveHistory, Player* player, const Player* whitePlayer, const Player* blackPlayer, bool whiteFlagged, bool blackFlagged, bool blackAtBottom, bool analyticsMode) {
        bool whiteCheck = board.check(false);
        bool blackCheck = board.check(true);
        bool checkmate = board.checkMate();
        bool whiteCheckmated = checkmate && whiteCheck;
        bool draw = board.draw();

        fps = deltaTime > 0.0f ? int(1.0f / deltaTime) : 0;
        turnText = board.getPlayerTurn() ? "Black to move" : "White to move";

        if (auto* ai = dynamic_cast<AIPlayer*>(player)) {
            lastAi = ai;
        }

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
            hasLastAi = true;
        }
        else {
            hasLastAi = false;
            lastAiOpening = false;
            lastAiSearchBlackToMove = false;
        }

        history = moveHistory;
        whiteTime = whitePlayer != nullptr ? whitePlayer->getTimeRemaining() : 0.0f;
        blackTime = blackPlayer != nullptr ? blackPlayer->getTimeRemaining() : 0.0f;
        whiteTimed = whitePlayer != nullptr && whitePlayer->isTimed();
        blackTimed = blackPlayer != nullptr && blackPlayer->isTimed();
        this->blackAtBottom = blackAtBottom;
        this->analyticsMode = analyticsMode;
        canUndoMove = !moveHistory.empty();
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

    float evalRailX() const {
        return panelX + 10.0f;
    }

    float sectionX() const {
        return analyticsMode ? panelX + evalRailW + 30.0f : panelX + 18.0f;
    }

    float sectionW() const {
        return analyticsMode ? panelW - evalRailW - 44.0f : panelW - 36.0f;
    }

    sf::FloatRect analyticsButtonBounds() const {
        constexpr float buttonW = 132.0f;
        constexpr float buttonH = 28.0f;
        float x = sectionX() + sectionW() - pad - buttonW;
        float y = 28.0f;

        return {{x, y}, {buttonW, buttonH}};
    }

    sf::FloatRect undoButtonBounds() const {
        constexpr float buttonW = 112.0f;
        constexpr float buttonH = 30.0f;
        float x = sectionX() + sectionW() - pad - buttonW;
        float y = 68.0f;

        return {{x, y}, {buttonW, buttonH}};
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

    void drawGameSection(sf::RenderWindow& window) {
        float y = 18.0f;
        float h = 92.0f;
        float x = sectionX() + pad;

        drawSection(window, y, h);
        drawText(window, "GAME", 13, x, y + 14.0f, mutedText, true);
        drawText(window, "FPS " + std::to_string(fps), 13, x + 58.0f, y + 14.0f, mutedText);
        drawText(window, turnText, 25, x, y + 36.0f, primaryText, true);
        drawText(window, statusText, 16, x, y + 68.0f, statusColor);
        drawButton(window, analyticsButtonBounds(), analyticsMode ? "Analytics On" : "Analytics Off", true, analyticsMode);

        if (analyticsMode) {
            drawButton(window, undoButtonBounds(), "Undo Move", canUndoMove);
        }
    }

    void drawEvalSection(sf::RenderWindow& window, float eval, bool mate, int depth) {
        float y = 126.0f;
        float h = 120.0f;
        float x = sectionX() + pad;

        drawSection(window, y, h);
        drawText(window, "EVALUATION", 13, x, y + 14.0f, mutedText, true);
        drawStatRow(window, "Score", formatEval(eval, mate), y + 46.0f);
        drawStatRow(window, "Depth", std::to_string(depth), y + 70.0f);
        drawText(window, eval >= 0.0f ? "White advantage" : "Black advantage", 15, x, y + 94.0f, eval >= 0.0f ? primaryText : mutedText);
    }

    void drawMaterialSection(sf::RenderWindow& window, const Board& board) {
        float y = analyticsMode ? 262.0f : 126.0f;
        float h = 154.0f;
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
            drawCapturedRow(window, "WHITE", whiteTime, whiteTimed, captures, true, y + 38.0f, materialDiff > 0 ? materialDiff : 0);
            drawCapturedRow(window, "BLACK", blackTime, blackTimed, captures, false, y + 92.0f, materialDiff < 0 ? -materialDiff : 0);
        } else {
            drawCapturedRow(window, "BLACK", blackTime, blackTimed, captures, false, y + 38.0f, materialDiff < 0 ? -materialDiff : 0);
            drawCapturedRow(window, "WHITE", whiteTime, whiteTimed, captures, true, y + 92.0f, materialDiff > 0 ? materialDiff : 0);
        }
    }

    void drawAiSection(sf::RenderWindow& window) {
        float y = 432.0f;
        float h = 162.0f;
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

        drawStatRow(window, "Depth", std::to_string(lastAiSearch.depth), y + 44.0f);
        drawStatRow(window, "Eval", formatEval(aiEval, lastAiSearch.mate), y + 62.0f);
        drawStatRow(window, "Evals", std::to_string(lastAiSearch.positionsEvaluated), y + 80.0f);
        drawStatRow(window, "Nodes/s", std::to_string(lastAiSearch.nodesPerSecond), y + 98.0f);
        drawStatRow(window, "TT", std::to_string(lastAiSearch.ttLookups), y + 116.0f);
        drawStatRow(window, "Time", std::to_string(lastAiSearch.timeTaken) + " ms", y + 134.0f);
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

    void drawMoveHistory(sf::RenderWindow& window) {
        float y = analyticsMode ? 610.0f : 296.0f;
        float h = float(height) - y - 20.0f;
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

    std::array<int, 16> buildCapturedCounts() const {
        std::array<int, 16> counts = fenCaptures;
        for (const MoveRecord& record : history) {
            if (record.capturedPiece != EMPTY) {
                counts[record.capturedPiece]++;
            }
        }
        return counts;
    }

    void drawCapturedRow(sf::RenderWindow& window, const std::string& label, float playerTime, bool playerTimed, const std::array<int, 16>& captures, bool whiteCaptured, float y, int materialPlus) {
        float x = sectionX() + pad;
        float rightX = sectionX() + sectionW() - pad;
        const std::array<Piece, 5> pieces = whiteCaptured
            ? std::array<Piece, 5>{BLACK_QUEEN, BLACK_ROOK, BLACK_BISHOP, BLACK_KNIGHT, BLACK_PAWN}
            : std::array<Piece, 5>{WHITE_QUEEN, WHITE_ROOK, WHITE_BISHOP, WHITE_KNIGHT, WHITE_PAWN};
        bool hasCaptures = false;

        drawText(window, label, 13, x, y, mutedText, true);
        if (materialPlus > 0) {
            drawText(window, "+" + std::to_string(materialPlus), 16, x + 58.0f, y - 2.0f, primaryText, true);
        }
        if (playerTimed) {
            drawTextRight(window, formatClock(playerTime), 24, rightX, y - 8.0f, clockColor(playerTime), true);
        }

        float iconSize = 34.0f;
        float stackStep = 12.0f;
        float groupGap = 11.0f;
        float iconX = x;
        float iconY = y + 18.0f;

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
            eval = std::clamp(eval, -10.0f, 10.0f);
            targetWhiteScore = 1.0f / (1.0f + std::exp(-eval / 2.0f));
        }

        float speed = 8.0f;
        animatedWhiteScore += (targetWhiteScore - animatedWhiteScore)
                             * std::clamp(deltaTime * speed, 0.0f, 1.0f);

        float innerWhiteHeight = ih * animatedWhiteScore;

        drawRoundedRect(window, {x, y}, {w, h}, w * 0.5f, dividerColor);
        drawRoundedRect(window, {ix, iy}, {iw, ih}, iw * 0.5f, sf::Color(18, 19, 21));

        if (innerWhiteHeight > 1.0f) {
            drawBottomRoundedRect(window, {ix, iy + ih - innerWhiteHeight}, {iw, innerWhiteHeight}, iw * 0.5f, sf::Color(230, 230, 230));
        }

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
