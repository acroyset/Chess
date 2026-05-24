//
// Created by Andreas Royset on 5/21/26.
//

#ifndef AIPLAYER_H
#define AIPLAYER_H

#include <algorithm>
#include <cmath>

#include "../Player.h"

#include "OpeningBook.h"
#include "TranspositionTable.h"
#include "Searcher.h"

const float MATE_SCORE = 1000000.0f;
class AIPlayer : public Player {
	Searcher* searcher = nullptr;

	int thinkingTime;
	int maxDepth = 32;

	SearchResult lastSearch{};

	bool started = false;
	std::chrono::time_point<std::chrono::steady_clock> endTime;
	int currentMoveTime = 0;
	bool currentSearchBlackToMove = false;
	bool lastSearchBlackToMove = false;
	bool lastMoveFromOpeningBook = false;

public:

	explicit AIPlayer(Searcher* searcher, int thinkingTime, float startingTime, float increment)
		: Player(startingTime, increment), searcher(searcher), thinkingTime(thinkingTime) {}

	std::optional<Move> selectMove(Board& board, sf::RenderWindow& window) override {

		if (!started) {
			currentSearchBlackToMove = board.getPlayerTurn();

			std::optional<Move> bookMove = getBookMove(board.computePolyglotHash());
			if (bookMove.has_value()) {
				lastSearch = SearchResult{};
				lastSearch.bestMove = bookMove.value();
				lastSearch.finished = true;
				lastSearchBlackToMove = currentSearchBlackToMove;
				lastMoveFromOpeningBook = true;
				return bookMove.value();
			}

			lastMoveFromOpeningBook = false;
			currentMoveTime = adaptiveThinkingTime();
			searcher->startSearch(board);

			endTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(currentMoveTime);

			started = true;

			return std::nullopt;

		}

		if (std::chrono::steady_clock::now() < endTime) return std::nullopt;

		lastSearch = searcher->getResult();
		lastSearchBlackToMove = currentSearchBlackToMove;
		lastMoveFromOpeningBook = false;

		started = false;

		return lastSearch.bestMove;
	}

	[[nodiscard]] SearchResult getLastSearch() const {
		return lastSearch;
	}
	[[nodiscard]] bool getLastSearchBlackToMove() const {
		return lastSearchBlackToMove;
	}
	[[nodiscard]] bool usedOpeningBook() const {
		return lastMoveFromOpeningBook;
	}

private:
	[[nodiscard]] int adaptiveThinkingTime() const {
		float remainingMs = getTimeRemaining()*1000;

		float safetyBuffer = 25;
		float usableMs = std::max(1.0f, remainingMs - safetyBuffer);
		float budget = usableMs*0.025f;

		return int(budget + getIncrimentTime()*1000);
	}
};

#endif
