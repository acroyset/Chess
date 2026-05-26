#ifndef STATUSICONATLAS_H
#define STATUSICONATLAS_H

#include <SFML/Graphics/Rect.hpp>

enum class StatusIcon {
    Crown,
    Draw,
    WhiteCheckmate,
    BlackCheckmate,
    WhiteClock,
    BlackClock
};

inline sf::IntRect statusIconCell(int column, int row) {
    constexpr int cellW = 86;
    constexpr int cellH = 80;
    return {{column * cellW, row * cellH}, {cellW, cellH}};
}

inline sf::IntRect statusIconRect(StatusIcon icon) {
    switch (icon) {
        case StatusIcon::Crown:
            return statusIconCell(0, 2);
        case StatusIcon::Draw:
            return statusIconCell(2, 5);
        case StatusIcon::WhiteCheckmate:
            return statusIconCell(3, 0);
        case StatusIcon::BlackCheckmate:
            return statusIconCell(4, 0);
        case StatusIcon::WhiteClock:
            return statusIconCell(3, 2);
        case StatusIcon::BlackClock:
            return statusIconCell(4, 2);
    }

    return {{0, 0}, {0, 0}};
}

#endif //STATUSICONATLAS_H
