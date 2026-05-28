//
// Created by Codex on 5/27/26.
//

#ifndef GAMERUNNERS_H
#define GAMERUNNERS_H

#include "../AI/Searcher.h"
#include "GameManager.h"

enum Result {
	NoResult,
	WhiteWin,
	Draw,
	BlackWin
};

struct ChessApp {
	static Result runHeadlessGame(GameManager& game, Searcher& searcher);
	static Result runRenderedGame(GameManager& game, Searcher& searcher);
};

#endif // GAMERUNNERS_H
