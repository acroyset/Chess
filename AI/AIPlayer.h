//
// Created by Andreas Royset on 5/21/26.
//

#ifndef AIPLAYER_H
#define AIPLAYER_H

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

public:

	explicit AIPlayer(Searcher* searcher, int thinkingTime) : searcher(searcher), thinkingTime(thinkingTime) {}

	std::optional<Move> selectMove(Board& board, sf::RenderWindow& window) override {

		if (!started) {

			std::optional<Move> bookMove = getBookMove(board.computePolyglotHash());
			if (bookMove.has_value()) {
				lastSearch = SearchResult{};
				lastSearch.bestMove = bookMove.value();
				lastSearch.finished = true;
				return bookMove.value();
			}

			searcher->startSearch(board);

			endTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(thinkingTime);

			started = true;

			return std::nullopt;

		}

		if (std::chrono::steady_clock::now() < endTime) return std::nullopt;

		lastSearch = searcher->getResult();

		started = false;

		return lastSearch.bestMove;
	}

	[[nodiscard]] SearchResult getLastSearch() const {
		return lastSearch;
	}
};

#endif