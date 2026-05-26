#include <algorithm>
#include <filesystem>
#include <iostream>

#include <SFML/Graphics/Image.hpp>

#include "../Render/StatusIconAtlas.h"

namespace {

enum class PieceIcon {
    WhiteKing,
    BlackKing
};

sf::IntRect pieceRect(PieceIcon piece, sf::Vector2u textureSize) {
    int cellW = int(textureSize.x) / 6;
    int cellH = int(textureSize.y) / 2;
    int y = piece == PieceIcon::BlackKing ? 1 : 0;

    return {{0, y * cellH}, {cellW, cellH}};
}

sf::Color blendOver(sf::Color src, sf::Color dst) {
    float alpha = float(src.a) / 255.0f;
    auto mix = [alpha](std::uint8_t s, std::uint8_t d) {
        return std::uint8_t(std::clamp(float(s) * alpha + float(d) * (1.0f - alpha), 0.0f, 255.0f));
    };

    return {mix(src.r, dst.r), mix(src.g, dst.g), mix(src.b, dst.b), 255};
}

void fillRect(sf::Image& image, sf::Vector2u position, sf::Vector2u size, sf::Color color) {
    sf::Vector2u imageSize = image.getSize();
    for (unsigned y = position.y; y < std::min(position.y + size.y, imageSize.y); y++) {
        for (unsigned x = position.x; x < std::min(position.x + size.x, imageSize.x); x++) {
            image.setPixel({x, y}, color);
        }
    }
}

void blitScaled(sf::Image& target, const sf::Image& source, sf::IntRect sourceRect, sf::Vector2u position, sf::Vector2u size) {
    sf::Vector2u targetSize = target.getSize();
    for (unsigned y = 0; y < size.y; y++) {
        unsigned targetY = position.y + y;
        if (targetY >= targetSize.y) continue;

        int sourceY = sourceRect.position.y + int(float(y) / float(size.y) * float(sourceRect.size.y));
        for (unsigned x = 0; x < size.x; x++) {
            unsigned targetX = position.x + x;
            if (targetX >= targetSize.x) continue;

            int sourceX = sourceRect.position.x + int(float(x) / float(size.x) * float(sourceRect.size.x));
            sf::Color src = source.getPixel({unsigned(sourceX), unsigned(sourceY)});
            if (src.a == 0) continue;

            sf::Color dst = target.getPixel({targetX, targetY});
            target.setPixel({targetX, targetY}, blendOver(src, dst));
        }
    }
}

void drawBoard(sf::Image& image, sf::Vector2u origin, unsigned tile) {
    const sf::Color light{235, 236, 208};
    const sf::Color dark{115, 149, 82};

    for (unsigned rank = 0; rank < 8; rank++) {
        for (unsigned file = 0; file < 8; file++) {
            fillRect(image, {origin.x + file * tile, origin.y + rank * tile}, {tile, tile}, (rank + file) % 2 == 0 ? light : dark);
        }
    }
}

void drawPiece(sf::Image& target, const sf::Image& pieces, PieceIcon piece, sf::Vector2u origin, unsigned tile, unsigned file, unsigned rank) {
    blitScaled(
        target,
        pieces,
        pieceRect(piece, pieces.getSize()),
        {origin.x + file * tile, origin.y + rank * tile},
        {tile, tile}
    );
}

void drawStatus(sf::Image& target, const sf::Image& icons, StatusIcon icon, sf::Vector2u origin, unsigned tile, unsigned file, unsigned rank) {
    unsigned iconSize = unsigned(float(tile) * 0.34f);
    unsigned margin = unsigned(float(tile) * 0.06f);
    blitScaled(
        target,
        icons,
        statusIconRect(icon),
        {
            origin.x + file * tile + tile - iconSize - margin,
            origin.y + rank * tile + margin
        },
        {iconSize, iconSize}
    );
}

} // namespace

int main() {
    std::filesystem::path sourceDir = CHESS_SOURCE_DIR;

    sf::Image pieces;
    if (!pieces.loadFromFile(sourceDir / "Render" / "ChessPieces.png")) {
        std::cerr << "Failed to load chess piece sheet\n";
        return 1;
    }

    sf::Image icons;
    if (!icons.loadFromFile(sourceDir / "icons.png")) {
        std::cerr << "Failed to load icon sheet\n";
        return 1;
    }

    constexpr unsigned boardSize = 560;
    constexpr unsigned tile = boardSize / 8;
    constexpr unsigned gap = 30;
    constexpr unsigned width = boardSize * 3 + gap * 4;
    constexpr unsigned height = boardSize * 2 + gap * 3;

    sf::Image preview;
    preview.resize({width, height}, sf::Color(22, 24, 27));

    sf::Vector2u whiteMatedOrigin{gap, gap};
    sf::Vector2u blackMatedOrigin{gap * 2 + boardSize, gap};
    sf::Vector2u drawOrigin{gap * 3 + boardSize * 2, gap};
    sf::Vector2u whiteTimeoutOrigin{gap, gap * 2 + boardSize};
    sf::Vector2u blackTimeoutOrigin{gap * 2 + boardSize, gap * 2 + boardSize};

    drawBoard(preview, whiteMatedOrigin, tile);
    drawPiece(preview, pieces, PieceIcon::BlackKing, whiteMatedOrigin, tile, 5, 1);
    drawStatus(preview, icons, StatusIcon::Crown, whiteMatedOrigin, tile, 5, 1);
    drawPiece(preview, pieces, PieceIcon::WhiteKing, whiteMatedOrigin, tile, 5, 6);
    drawStatus(preview, icons, StatusIcon::WhiteCheckmate, whiteMatedOrigin, tile, 5, 6);

    drawBoard(preview, blackMatedOrigin, tile);
    drawPiece(preview, pieces, PieceIcon::WhiteKing, blackMatedOrigin, tile, 2, 6);
    drawStatus(preview, icons, StatusIcon::Crown, blackMatedOrigin, tile, 2, 6);
    drawPiece(preview, pieces, PieceIcon::BlackKing, blackMatedOrigin, tile, 5, 1);
    drawStatus(preview, icons, StatusIcon::BlackCheckmate, blackMatedOrigin, tile, 5, 1);

    drawBoard(preview, drawOrigin, tile);
    drawPiece(preview, pieces, PieceIcon::BlackKing, drawOrigin, tile, 2, 2);
    drawStatus(preview, icons, StatusIcon::Draw, drawOrigin, tile, 2, 2);
    drawPiece(preview, pieces, PieceIcon::WhiteKing, drawOrigin, tile, 5, 5);
    drawStatus(preview, icons, StatusIcon::Draw, drawOrigin, tile, 5, 5);

    drawBoard(preview, whiteTimeoutOrigin, tile);
    drawPiece(preview, pieces, PieceIcon::BlackKing, whiteTimeoutOrigin, tile, 5, 1);
    drawStatus(preview, icons, StatusIcon::Crown, whiteTimeoutOrigin, tile, 5, 1);
    drawPiece(preview, pieces, PieceIcon::WhiteKing, whiteTimeoutOrigin, tile, 2, 6);
    drawStatus(preview, icons, StatusIcon::WhiteClock, whiteTimeoutOrigin, tile, 2, 6);

    drawBoard(preview, blackTimeoutOrigin, tile);
    drawPiece(preview, pieces, PieceIcon::WhiteKing, blackTimeoutOrigin, tile, 2, 6);
    drawStatus(preview, icons, StatusIcon::Crown, blackTimeoutOrigin, tile, 2, 6);
    drawPiece(preview, pieces, PieceIcon::BlackKing, blackTimeoutOrigin, tile, 5, 1);
    drawStatus(preview, icons, StatusIcon::BlackClock, blackTimeoutOrigin, tile, 5, 1);

    std::filesystem::path outputPath = sourceDir / "icon_board_preview.png";
    if (!preview.saveToFile(outputPath)) {
        std::cerr << "Failed to save " << outputPath << '\n';
        return 1;
    }

    std::cout << outputPath << '\n';
    return 0;
}
