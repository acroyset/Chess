//
// Created by Andreas Royset on 5/22/26.
//

#ifndef SEARCHER_H
#define SEARCHER_H

#include <atomic>
#include <thread>
#include <mutex>
#include <optional>
#include <chrono>
#include <algorithm>
#include <array>
#include <cmath>

#include "../Board.h"
#include "../BoardState.h"
#include "TranspositionTable.h"
#include "Evalutation.h"


constexpr float SEARCH_INF        = 1000000000.0f;
constexpr float SEARCH_MATE_SCORE = 1000000.0f;
constexpr int   MAX_DEPTH         = 64;

struct SearchResult {
    std::optional<Move> bestMove{};
    float eval = 0.0f;
    int depth = 0;
    int positionsEvaluated = 0;
    int nodesPerSecond = 0;
    int ttLookups = 0;
    int timeTaken = 0;
    bool mate = false;
    bool finished = false;
};

struct ScoredMove {
    Move move;
    float score{};
};

struct PVTable {
    Move pvLine[MAX_DEPTH][MAX_DEPTH]{};
    int  pvLength[MAX_DEPTH]{};

    void clear() {
        for (int & i : pvLength) i = 0;
    }

    void updatePV(int ply, const Move& move) {
        pvLine[ply][ply] = move;
        int childLen = pvLength[ply + 1];
        for (int i = ply + 1; i < ply + 1 + childLen && i < MAX_DEPTH; i++) {
            pvLine[ply][i] = pvLine[ply + 1][i];
        }
        pvLength[ply] = 1 + childLen;
    }

    // Best move the PV recommends at a given ply (nullopt if none stored)
    [[nodiscard]] Move pvMove(int ply) const {
        if (ply < MAX_DEPTH && pvLength[ply] > ply) return pvLine[ply][ply];
        return Move{};   // default-constructed "no move"
    }

    [[nodiscard]] bool hasPVMove(int ply) const {
        return ply < MAX_DEPTH && pvLength[ply] > ply;
    }
};


class Searcher {
    Board board;
    TranspositionTable tt;

    std::thread thread;
    std::atomic<bool> abort{false};
    std::atomic<bool> searching{false};

    mutable std::mutex resultMutex;
    SearchResult result{};

    std::atomic<int> nodesSinceTimeCheck{0};
    std::atomic<int> positionsEvaluated{0};
    std::atomic<int> ttLookups{0};

    std::chrono::steady_clock::time_point startTime;

    int threadCount = std::max(1, int(std::thread::hardware_concurrency()));

public:
    Searcher() = default;

    ~Searcher() {
        endSearch();
    }

    Searcher(const Searcher&) = delete;
    Searcher& operator=(const Searcher&) = delete;

    void startSearch(const Board& startBoard);

    SearchResult endSearch();

    [[nodiscard]] SearchResult getResult() const { std::lock_guard lock(resultMutex); return result; }
    [[nodiscard]] bool isSearching() const { return searching; }

private:

    struct ThreadData {
        Board    board;
        Move     killerMoves[MAX_DEPTH][2]{};
        int      history[64][64]{};
        PVTable  pv{};
    };

    void runSearch();

    void searchThread(ThreadData& td, int threadIndex);
    float search(Board& board, int depth, float alpha, float beta, int ply, Move killerMoves[MAX_DEPTH][2], int history[64][64], PVTable& pv);
    float searchCaptures(Board& board, float alpha, float beta, int ply, Move killerMoves[MAX_DEPTH][2], int history[64][64]);

    [[nodiscard]] static float guessMoveScore(const Board& board, const Move& move, const Move& ttMove, const Move killerMoves[2], const int history[64][64]);
    static void sortMoves(Board& board, MoveList& moves, const Move& ttMove, const Move killerMoves[2], const int history[64][64]);

    void finish();
    void updateResult(const Move& bestMove, float bestEval, int depth, bool finished);

    bool shouldAbort() {
        int n = nodesSinceTimeCheck.fetch_add(1, std::memory_order_relaxed);
        if ((n & 4095) != 0) return false;
        return abort.load(std::memory_order_relaxed);
    }
    static int averageNodesPerSecond(int nodes, int milliseconds) {
        if (milliseconds <= 0) return 0;
        return int(std::round(double(nodes) * 1000.0 / double(milliseconds)));
    }
    void incrementPositions() {
        positionsEvaluated.fetch_add(1, std::memory_order_relaxed);
    }
    void incrementTTLookups() {
        ttLookups.fetch_add(1, std::memory_order_relaxed);
    }
};

#endif
