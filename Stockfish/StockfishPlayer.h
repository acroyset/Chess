//
// Created by Codex on 5/27/26.
//

#ifndef STOCKFISHPLAYER_H
#define STOCKFISHPLAYER_H

#include <atomic>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "../Player.h"

struct StockfishSearchResult {
    std::optional<Move> bestMove{};
    std::string bestMoveUci;
    float eval = 0.0f;
    int depth = 0;
    bool mate = false;
    bool available = false;
    bool finished = false;
    bool timedOut = false;
    bool blackToMove = false;
    uint64_t boardHash = 0;
    std::string enginePath;
    std::string error;
};

std::string moveToUCI(Move move);

class StockfishPlayer : public Player {
    std::thread thread;
    std::atomic<bool> searching{false};
    mutable std::mutex resultMutex;
    StockfishSearchResult currentResult{};
    StockfishSearchResult lastResult{};
    std::string rootFen;
    std::vector<Move> playedMoves;
    int moveTimeMs = 700;
    bool started = false;

public:
    explicit StockfishPlayer(float startingTime, float increment, int moveTimeMs = 700, const std::string& name = "")
        : Player(startingTime, increment, name.empty() ? "Stockfish" : name), moveTimeMs(moveTimeMs) {}
    explicit StockfishPlayer(int moveTimeMs = 700, const std::string& name = "")
        : Player(name.empty() ? "Stockfish" : name), moveTimeMs(moveTimeMs) {}
    ~StockfishPlayer() override;

    StockfishPlayer(const StockfishPlayer&) = delete;
    StockfishPlayer& operator=(const StockfishPlayer&) = delete;

    std::optional<Move> selectMove(Board& board, sf::RenderWindow& window) override;
    void resetInput() override;
    void setGameHistory(const std::string& startFen, const std::vector<Move>& moves);

    [[nodiscard]] StockfishSearchResult getLastSearch() const;
    [[nodiscard]] bool isSearching() const { return searching.load(); }
    [[nodiscard]] static bool isAvailable();
};

#endif // STOCKFISHPLAYER_H
