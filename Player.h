#ifndef PLAYER_H
#define PLAYER_H

#include <algorithm>
#include <optional>
#include <SFML/Graphics.hpp>
#include "Board.h"

class Player {
    float timeRemaining = 0.0f;
	float increment = 0.0f;

public:
    explicit Player(float startingTime, float increment)
        : timeRemaining(startingTime), increment(increment) {}

	virtual ~Player() = default;

	virtual std::optional<Move> selectMove(
		Board& board,
		sf::RenderWindow& window
	) = 0;

	void tickClock(float deltaTime) {
		timeRemaining = std::max(0.0f, timeRemaining - deltaTime);
	}

	[[nodiscard]] float getTimeRemaining() const {
		return timeRemaining;
	}
	[[nodiscard]] float getIncrimentTime() const {
		return increment;
	}
	[[nodiscard]] bool outOfTime() const {
		return timeRemaining <= 0.0f;
	}
	void incrementTime() {
		timeRemaining += increment;
	}

	[[nodiscard]] virtual MoveList getShownMoves() const {
		return {};
	}
};

#endif
