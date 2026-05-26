//
// Created by Andreas Royset on 5/21/26.
//

#ifndef AIPLAYER_H
#define AIPLAYER_H

#include <algorithm>
#include <chrono>
#include <cmath>

#include "../Player.h"

#include "OpeningBook.h"
#include "TranspositionTable.h"
#include "Searcher.h"

const float MATE_SCORE = 1000000.0f;
class AIPlayer : public Player {
	Searcher* searcher = nullptr;

	int thinkingTimeMs = 1000;

	int maxDepth = 32;

	SearchResult lastSearch{};
	int lastThinkingTimeMs = 0;

	bool started = false;
	std::chrono::time_point<std::chrono::steady_clock> searchStartTime;
	std::chrono::time_point<std::chrono::steady_clock> endTime;
	bool currentSearchBlackToMove = false;
	bool lastSearchBlackToMove = false;
	bool lastMoveFromOpeningBook = false;

public:

	explicit AIPlayer(Searcher* searcher, float startingTime, float increment)
		: Player(startingTime, increment), searcher(searcher) {}
	explicit AIPlayer(Searcher* searcher, float fixedThinkingSeconds)
		: thinkingTimeMs(std::max(1, int(std::round(fixedThinkingSeconds * 1000.0f)))), searcher(searcher) {}
	explicit AIPlayer(Searcher* searcher, double fixedThinkingSeconds)
		: AIPlayer(searcher, float(fixedThinkingSeconds)) {}

	std::optional<Move> selectMove(Board& board, sf::RenderWindow& window) override {

		if (!started) {
			currentSearchBlackToMove = board.getPlayerTurn();

			std::optional<Move> bookMove = getBookMove(board.computePolyglotHash());
			if (bookMove.has_value()) {
				lastSearch = SearchResult{};
				lastSearch.bestMove = bookMove.value();
				lastSearch.finished = true;
				lastThinkingTimeMs = 0;
				lastSearchBlackToMove = currentSearchBlackToMove;
				lastMoveFromOpeningBook = true;
				return bookMove.value();
			}

			lastMoveFromOpeningBook = false;
			thinkingTimeMs = adaptiveThinkingTime();
			searchStartTime = std::chrono::steady_clock::now();
			searcher->startSearch(board);

			endTime = searchStartTime + std::chrono::milliseconds(thinkingTimeMs);

			started = true;

			return std::nullopt;

		}

		if (std::chrono::steady_clock::now() < endTime) return std::nullopt;

		auto finishedThinkingAt = std::chrono::steady_clock::now();
		lastThinkingTimeMs = int(std::chrono::duration_cast<std::chrono::milliseconds>(
			finishedThinkingAt - searchStartTime
		).count());
		lastSearch = searcher->getResult();
		lastSearchBlackToMove = currentSearchBlackToMove;
		lastMoveFromOpeningBook = false;

		started = false;

		return lastSearch.bestMove;
	}

	void resetInput() override {
		started = false;
		lastMoveFromOpeningBook = false;
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
	[[nodiscard]] int getLastThinkingTimeMs() const {
		return lastThinkingTimeMs;
	}

private:
	[[nodiscard]] int adaptiveThinkingTime() const {

		if (!isTimed()) return thinkingTimeMs;

		float remainingMs = getTimeRemaining()*1000;

		float safetyBuffer = 25;
		float usableMs = std::max(1.0f, remainingMs - safetyBuffer);
		float budget = usableMs*0.025f;

		return std::max(1, int(budget + getIncrimentTime()*1000));
	}
};

#endif
