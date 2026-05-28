#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>

#include "AI/AIPlayer.h"
#include "AI/OpeningBook.h"
#include "AI/Searcher.h"
#include "Game/GameRunners.h"
#include "Game/GameManager.h"

int main() {
    openingBook.clear();
    loadOpeningBook("Openings/gm2001.bin");
    loadOpeningBook("Openings/komodo.bin");
    loadOpeningBook("Openings/rodent.bin");

    Searcher searcher;

    std::unique_ptr<Player> whitePlayer = std::make_unique<AIPlayer>(&searcher, 10.0f * 60.0f, 5.0f);
    std::unique_ptr<Player> blackPlayer = std::make_unique<AIPlayer>(&searcher, 10.0f * 60.0f, 5.0f);

    GameManager game(std::move(whitePlayer), std::move(blackPlayer));

    Result result = ChessApp::runRenderedGame(game, searcher);
}
