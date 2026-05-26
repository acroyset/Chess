#include <array>
#include <filesystem>
#include <iostream>
#include <string>

#include <SFML/Graphics/Image.hpp>

#include "../Render/StatusIconAtlas.h"

namespace {

struct IconCell {
    const char* name;
    unsigned column;
    unsigned row;
};

bool hasVisiblePixels(const sf::Image& image, unsigned cellW, unsigned cellH, IconCell cell) {
    constexpr unsigned margin = 6;
    unsigned visiblePixels = 0;

    unsigned startX = cell.column * cellW + margin;
    unsigned endX = (cell.column + 1) * cellW - margin;
    unsigned startY = cell.row * cellH + margin;
    unsigned endY = (cell.row + 1) * cellH - margin;

    for (unsigned y = startY; y < endY; y++) {
        for (unsigned x = startX; x < endX; x++) {
            if (image.getPixel({x, y}).a > 16) {
                visiblePixels++;
            }
        }
    }

    return visiblePixels > 1000;
}

struct IconStats {
    unsigned visiblePixels = 0;
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
};

IconStats statsForRect(const sf::Image& image, sf::IntRect rect) {
    constexpr int margin = 6;
    IconStats stats;

    int startX = rect.position.x + margin;
    int endX = rect.position.x + rect.size.x - margin;
    int startY = rect.position.y + margin;
    int endY = rect.position.y + rect.size.y - margin;

    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {
            sf::Color pixel = image.getPixel({unsigned(x), unsigned(y)});
            if (pixel.a <= 32) continue;

            stats.visiblePixels++;
            stats.r += pixel.r;
            stats.g += pixel.g;
            stats.b += pixel.b;
        }
    }

    if (stats.visiblePixels > 0) {
        stats.r /= double(stats.visiblePixels);
        stats.g /= double(stats.visiblePixels);
        stats.b /= double(stats.visiblePixels);
    }

    return stats;
}

double luminance(const IconStats& stats) {
    return stats.r * 0.2126 + stats.g * 0.7152 + stats.b * 0.0722;
}

bool isRedIcon(const IconStats& stats) {
    return stats.r > stats.g + 45.0 && stats.r > stats.b + 45.0;
}

} // namespace

int main() {
    std::filesystem::path iconsPath = std::filesystem::path(CHESS_SOURCE_DIR) / "icons.png";

    sf::Image image;
    if (!image.loadFromFile(iconsPath)) {
        std::cerr << "Failed to load " << iconsPath << '\n';
        return 1;
    }

    sf::Vector2u size = image.getSize();
    if (size.x != 519 || size.y != 481) {
        std::cerr << "Unexpected icons.png size: " << size.x << "x" << size.y << '\n';
        return 1;
    }

    constexpr unsigned columns = 6;
    constexpr unsigned rows = 6;
    unsigned cellW = size.x / columns;
    unsigned cellH = size.y / rows;

    if (cellW != 86 || cellH != 80) {
        std::cerr << "Unexpected icon grid cell size: " << cellW << "x" << cellH << '\n';
        return 1;
    }

    constexpr std::array<IconCell, 6> usedIcons{{
        {"win crown", 0, 2},
        {"draw half", 2, 5},
        {"white checkmate hash", 3, 0},
        {"black checkmate hash", 4, 0},
        {"white timeout clock", 3, 2},
        {"black timeout clock", 4, 2},
    }};

    for (IconCell cell : usedIcons) {
        if (!hasVisiblePixels(image, cellW, cellH, cell)) {
            std::cerr << "Icon cell appears blank: " << cell.name
                      << " at column " << cell.column
                      << ", row " << cell.row << '\n';
            return 1;
        }
    }

    IconStats whiteHash = statsForRect(image, statusIconRect(StatusIcon::WhiteCheckmate));
    IconStats blackHash = statsForRect(image, statusIconRect(StatusIcon::BlackCheckmate));
    IconStats whiteClock = statsForRect(image, statusIconRect(StatusIcon::WhiteClock));
    IconStats blackClock = statsForRect(image, statusIconRect(StatusIcon::BlackClock));
    IconStats redHash = statsForRect(image, statusIconCell(5, 2));

    if (isRedIcon(whiteHash) || isRedIcon(blackHash)) {
        std::cerr << "Checkmate icons are mapped to the red hash cell\n";
        return 1;
    }

    if (!isRedIcon(redHash)) {
        std::cerr << "Red hash guard cell is not red; icon atlas layout changed\n";
        return 1;
    }

    if (luminance(whiteHash) <= luminance(blackHash) + 35.0) {
        std::cerr << "White and black checkmate icons are not color-specific\n";
        return 1;
    }

    if (luminance(whiteClock) <= luminance(blackClock) + 35.0) {
        std::cerr << "White and black clock icons are not color-specific\n";
        return 1;
    }

    return 0;
}
