#ifndef PLAYER_H
#define PLAYER_H

#include <optional>
#include <SFML/Graphics.hpp>
#include "Board.h"

class Player {
public:
	virtual ~Player() = default;

	virtual std::optional<Move> selectMove(
		Board& board,
		sf::RenderWindow& window
	) = 0;

	[[nodiscard]] virtual MoveList getShownMoves() const {
		return {};
	}
};

#endif