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
#include <vector>
#include <algorithm>

#include "../Board.h"
#include "../BoardState.h"
#include "TranspositionTable.h"
#include "Evalutation.h"


constexpr float SEARCH_INF = 1000000000.0f;
constexpr float SEARCH_MATE_SCORE = 1000000.0f;
constexpr int MAX_DEPTH = 32;

inline float scoreToTT(float score, int ply) {
    if (score > SEARCH_MATE_SCORE - 1000)
        return score + float(ply);

    if (score < -SEARCH_MATE_SCORE + 1000)
        return score - float(ply);

    return score;
}
inline float scoreFromTT(float score, int ply) {
    if (score > SEARCH_MATE_SCORE - 1000)
        return score - float(ply);

    if (score < -SEARCH_MATE_SCORE + 1000)
        return score + float(ply);

    return score;
}

struct SearchResult {
    std::optional<Move> bestMove{};
    float eval = 0.0f;
    int depth = 0;
    int positionsEvaluated = 0;
    int ttLookups = 0;
    int timeTaken = 0;
    bool mate = false;
    bool finished = false;
};

struct ScoredMove {
    Move move;
    float score;
};

class Searcher {
    Board board;
    TranspositionTable tt;

    Move killerMoves[MAX_DEPTH][2]{};
    int history[64][64]{};

    std::thread thread;
    std::atomic<bool> abort{false};
    std::atomic<bool> searching{false};

    mutable std::mutex resultMutex;
    SearchResult result{};

    std::atomic<int> nodesSinceTimeCheck{0};
    std::atomic<int> positionsEvaluated{0};
    std::atomic<int> ttLookups{0};

    std::chrono::steady_clock::time_point startTime;

    int threadCount = int(std::thread::hardware_concurrency());

public:
    Searcher() = default;

    ~Searcher() {
        endSearch();
    }

    Searcher(const Searcher&) = delete;
    Searcher& operator=(const Searcher&) = delete;

    void startSearch(const Board& startBoard) {
        endSearch();

        positionsEvaluated = 0;
        ttLookups = 0;

        {
            std::lock_guard lock(resultMutex);
            result = SearchResult{};
        }

        board.set(startBoard);
        abort = false;
        searching = true;
        nodesSinceTimeCheck = 0;
        startTime = std::chrono::steady_clock::now();

        thread = std::thread([this]() {
            runSearch();
        });
    }

    SearchResult endSearch() {
        abort = true;

        if (thread.joinable()) {
            thread.join();
        }

        searching = false;

        std::lock_guard lock(resultMutex);
        return result;
    }

    [[nodiscard]] SearchResult getResult() const {
        std::lock_guard lock(resultMutex);
        return result;
    }

    [[nodiscard]] bool isSearching() const {
        return searching;
    }

private:

    struct ThreadData {
        Board board;
        Move killerMoves[MAX_DEPTH][2]{};
        int history[64][64]{};
    };

    void runSearch() {
        MoveList moves;
        board.getValidMoves(moves);

        if (moves.count == 0) {
            finish();
            return;
        }

        // Each thread runs a full independent iterative deepening search
        std::vector<std::thread> workers;
        std::vector<ThreadData> threadData(threadCount);

        // Initialize each thread's board copy
        for (int t = 0; t < threadCount; t++) {
            threadData[t].board.set(board);
        }

        // Launch worker threads — each does full iterative deepening
        workers.reserve(threadCount);
        for (int t = 0; t < threadCount; t++) {
            workers.emplace_back([this, &threadData, t]() {
                searchThread(threadData[t], t);
            });
        }

        for (auto& w : workers) {
            if (w.joinable()) w.join();
        }

        finish();
    }

    void searchThread(ThreadData& td, int threadIndex) {
        MoveList moves;
        td.board.getValidMoves(moves);

        if (moves.count == 0) return;

        Move bestMove = moves[0];
        float bestEval = -SEARCH_INF;

        // Offset starting depth per thread to reduce redundant work
        // Thread 0 starts at 1, thread 1 at 2, etc. (they all keep going)
        int startDepth = 1 + (threadIndex % 4);

        for (int depth = startDepth; depth <= MAX_DEPTH; depth++) {
            sortMoves(td.board, moves, bestMove, td.killerMoves[0], td.history);
            putBestMoveFirst(moves, bestMove);

            Move currentBestMove = bestMove;
            float currentBestEval = -SEARCH_INF;
            float alpha = -SEARCH_INF;

            for (int i = 0; i < moves.count; i++) {
                if (abort.load()) return;

                const Move& move = moves[i];
                td.board.move(move);
                float eval = -search(td.board, depth - 1, -SEARCH_INF, -alpha,
                                     1, td.killerMoves, td.history);
                td.board.undoMove();

                if (abort.load()) return;

                if (eval > currentBestEval) {
                    currentBestEval = eval;
                    currentBestMove = move;
                }
                alpha = std::max(alpha, eval);
            }

            bestMove = currentBestMove;
            bestEval = currentBestEval;

            // Only thread 0 updates the result (it's the "main" thread)
            if (threadIndex == 0) {
                updateResult(bestMove, bestEval, depth, false);
            }
        }
    }

    float search(Board& board, int depth, float alpha, float beta, int ply, Move killerMoves[MAX_DEPTH][2], int history[64][64]) {
        if (shouldAbort()) {
            abort = true;
            return alpha;
        }

        float originalAlpha = alpha;
        float originalBeta = beta;

        if (depth == 0) {
            return searchCaptures(board, alpha, beta, ply, killerMoves, history);
        }

        if (board.draw50Move() || board.drawRepetition()) {
            return 0.0f;
        }

        uint64_t hash = board.getHash();

        Move ttMove{};
        bool hasTTMove = false;

        Entry entry;
        entry = tt.find(hash);

        if (entry.flag != FAIL && entry.hash == hash) {
            hasTTMove = true;
            ttMove = entry.bestMove;
            float ttEval = scoreFromTT(entry.eval, ply);

            if (entry.depth >= depth) {
                incrementTTLookups();

                if (entry.flag == EXACT) return ttEval;

                if (entry.flag == LOWERBOUND) {
                    alpha = std::max(alpha, ttEval);
                } else if (entry.flag == UPPERBOUND) {
                    beta = std::min(beta, ttEval);
                }

                if (alpha >= beta) return ttEval;
            }
        }

        MoveList moves;
        board.getValidMoves(moves);

        if (moves.count == 0) {
            if (board.checkMate()) {
                return -SEARCH_MATE_SCORE + float(ply);
            }
            return 0.0f;
        }

        sortMoves(board, moves, hasTTMove ? ttMove : Move{}, killerMoves[ply], history);

        Move bestMove = moves[0];
        float bestEval = -SEARCH_INF;

        for (int i = 0; i < moves.count; i++) {
            Move move = moves[i];
            board.move(move);
            float eval = -search(board, depth - 1, -beta, -alpha, ply + 1, killerMoves, history);
            board.undoMove();

            if (abort) {
                return alpha;
            }

            if (eval > bestEval) {
                bestEval = eval;
                bestMove = move;
            }

            alpha = std::max(alpha, eval);

            if (alpha >= beta) {

                if (!move.isCapture()) {
                    if (!(move == killerMoves[ply][0])) {
                        killerMoves[ply][1] = killerMoves[ply][0];
                        killerMoves[ply][0] = move;
                    }

                    history[move.starting().index][move.target().index] += depth * depth;
                }

                break;
            }
        }

        if (!abort) {
            Entry newEntry{};
            newEntry.hash = hash;
            newEntry.eval = scoreToTT(bestEval, ply);;
            newEntry.depth = int8_t(depth);
            newEntry.bestMove = bestMove;

            if (bestEval <= originalAlpha) {
                newEntry.flag = UPPERBOUND;
            } else if (bestEval >= originalBeta) {
                newEntry.flag = LOWERBOUND;
            } else {
                newEntry.flag = EXACT;
            }

            tt.store(newEntry);
        }

        return bestEval;
    }

    float searchCaptures(Board& board, float alpha, float beta, int ply, Move killerMoves[MAX_DEPTH][2], int history[64][64]){
        if (shouldAbort()) {
            abort = true;
            return alpha;
        }

        if (board.draw50Move() || board.drawRepetition()) {
            return 0.0f;
        }

        float standPat = evaluateBoard(board);
        incrementPositions();

        if (standPat >= beta) {
            return standPat;
        }

        alpha = std::max(alpha, standPat);

        MoveList moves;
        board.getCaptureMoves(moves);
        sortMoves(board, moves, Move{}, killerMoves[ply], history);

        for (int i = 0; i < moves.count; i++) {
            Move move = moves[i];
            board.move(move);
            float eval = -searchCaptures(board, -beta, -alpha, ply+1, killerMoves, history);
            board.undoMove();

            if (abort) return alpha;

            if (eval >= beta) {
                return beta;
            }

            alpha = std::max(alpha, eval);
        }

        return alpha;
    }

    [[nodiscard]] static float guessMoveScore(Board& board, const Move& move, const Move& ttMove, const Move killerMoves[2], const int history[64][64]) {
        if (move == ttMove)
            return 1'000'000.0f;

        Piece attacker = board.getPiece(move.starting());
        Piece victim = board.getPiece(move.target());

        float score = 0.0f;

        // Promotions
        if (move.isPromotion()) {
            uint8_t promo = move.promotionPiece();

            switch (promo) {
                case 0:
                    score += 900'000.0f;
                    break;

                case 1:
                    score += 500'000.0f;
                    break;

                case 2:
                case 3:
                    score += 300'000.0f;
                    break;

                default:
                    score += 250'000.0f;
                    break;
            }
        }

        // MVV-LVA captures
        if (move.isCapture()) {
            score += 100'000.0f;

            score += 100.0f * pieceValueMg(victim);
            score -= pieceValueMg(attacker);

            if (attacker == WHITE_PAWN ||
                attacker == BLACK_PAWN) {
                score += 20.0f;
            }
        }
        else {
            // Killer moves
            if (move == killerMoves[0]) {
                score += 80'000.0f;
            }
            else if (move == killerMoves[1]) {
                score += 70'000.0f;
            }

            // History heuristic
            score += float(
                history[move.starting().index]
                       [move.target().index]
            );
        }

        return score;
    }

    static void sortMoves(Board& board, MoveList& moves, const Move& ttMove, const Move killerMoves[2], const int history[64][64]) {
        std::array<ScoredMove, 256> scoredMoves;

        for (int i = 0; i < moves.count; i++) {
            Move move = moves[i];
            scoredMoves[i] = {
                move,
                guessMoveScore(
                    board,
                    move,
                    ttMove,
                    killerMoves,
                    history
                )
            };
        }

        std::sort(
            scoredMoves.begin(),
            scoredMoves.begin() + moves.count,
            [](const ScoredMove& a, const ScoredMove& b) {
                return a.score > b.score;
            }
        );

        for (size_t i = 0; i < moves.count; i++) {
            moves[i] = scoredMoves[i].move;
        }
    }

    static void putBestMoveFirst(MoveList& moves, const Move& bestMove) {
        int idx = -1;
        for (int i = 0; i < moves.count; i++) {
            if (moves[i] == bestMove) {
                idx = i;
                break;
            }
        }

        if (idx != -1) {
            std::swap(moves.moves[idx], moves.moves[0]);
        }
    }

    bool shouldAbort() {
        int n = nodesSinceTimeCheck.fetch_add(1, std::memory_order_relaxed);

        if ((n & 4095) != 0)
            return false;

        return abort.load(std::memory_order_relaxed);
    }

    void updateResult(const Move& bestMove, float bestEval, int depth, bool finished) {
        std::lock_guard lock(resultMutex);

        result.bestMove = bestMove;
        result.depth = depth;
        result.finished = finished;

        result.positionsEvaluated = positionsEvaluated.load();
        result.ttLookups = ttLookups.load();

        result.mate = std::abs(bestEval) > SEARCH_MATE_SCORE - 1000;

        if (result.mate) {
            float matePly = round(SEARCH_MATE_SCORE - abs(bestEval))+1;
            result.eval = (std::signbit(bestEval) ? -1.0f : 1.0f) * (matePly + 1) / 2;
        } else {
            result.eval = bestEval;
        }

        result.timeTaken =
            int(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime
            ).count());
    }

    void finish() {
        {
            std::lock_guard lock(resultMutex);

            result.positionsEvaluated = positionsEvaluated.load();
            result.ttLookups = ttLookups.load();

            result.finished = true;
            result.timeTaken =
                int(std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - startTime
                ).count());

        }

        searching = false;
    }

    void incrementPositions() {
        positionsEvaluated.fetch_add(1, std::memory_order_relaxed);
    }
    void incrementTTLookups() {
        ttLookups.fetch_add(1, std::memory_order_relaxed);
    }
};

#endif
