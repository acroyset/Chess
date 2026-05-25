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

// SEE piece values (centipawns-ish, integers for speed)
constexpr int SEE_VAL[16] = {
    0,    // EMPTY
    100,  // WHITE_PAWN
    320,  // WHITE_KNIGHT
    330,  // WHITE_BISHOP
    500,  // WHITE_ROOK
    900,  // WHITE_QUEEN
    20000,// WHITE_KING
    0, 0, // unused
    100,  // BLACK_PAWN
    320,  // BLACK_KNIGHT
    330,  // BLACK_BISHOP
    500,  // BLACK_ROOK
    900,  // BLACK_QUEEN
    20000,// BLACK_KING
    0
};

inline int seeValue(Piece piece) {
    int idx = int(piece);
    return idx >= 0 && idx < 16 ? SEE_VAL[idx] : 0;
}

inline float scoreToTT(float score, int ply) {
    if (score > SEARCH_MATE_SCORE - 1000)  return score + float(ply);
    if (score < -SEARCH_MATE_SCORE + 1000) return score - float(ply);
    return score;
}
inline float scoreFromTT(float score, int ply) {
    if (score > SEARCH_MATE_SCORE - 1000)  return score - float(ply);
    if (score < -SEARCH_MATE_SCORE + 1000) return score + float(ply);
    return score;
}

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

// -----------------------------------------------------------------------
// Principal Variation table (triangular array)
//
// pvLine[ply] holds the best move at that ply.
// pvLength[ply] is how many moves are stored from ply onward.
//
// After a new best move is found at ply N, updatePV() copies:
//   pvLine[N]      = bestMove
//   pvLine[N+1..] = child's PV (pvLine[N+1..pvLength[N+1]])
// -----------------------------------------------------------------------
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

// -----------------------------------------------------------------------
// Static Exchange Evaluation
// Returns estimated material gain/loss for a capture on 'toSq'.
// Positive = winning capture, negative = losing capture.
// -----------------------------------------------------------------------
inline int see(const Board& board, Move move) {
    // Only meaningful for captures
    if (!move.isCapture()) return 0;

    int toIdx   = move.target().index;
    int fromIdx = move.starting().index;

    Piece victim   = board.getPiece(move.target());
    Piece attacker = board.getPiece(move.starting());

    if (attacker == EMPTY) return 0;

    if (victim == EMPTY) {
        if (!move.isEnPassant()) return 0;
        victim = (attacker == WHITE_PAWN) ? BLACK_PAWN : WHITE_PAWN;
    }

    int gain[32];
    int d = 0;
    gain[d] = seeValue(victim);

    // Build an occupancy mask — iterate piece bits
    uint64_t occ = 0;
    for (int p = 1; p <= 14; p++) {
        if (p == 7 || p == 8) continue;
        occ |= board.getPieceBits(static_cast<Piece>(p));
    }
    occ &= ~(1ULL << fromIdx); // remove the moving piece

    bool sideToMove = (attacker >> 3) != 0; // toggled below so the opponent recaptures first

    // Diagonal attackers: bishops + queens
    constexpr int diagDirs[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
    // Straight attackers: rooks + queens
    constexpr int straightDirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};

    auto leastValuableAttacker = [&](bool black) -> std::pair<int,int> {
        // Returns {seeValue, squareIndex} of LVA, or {-1,-1} if none
        int bestVal = INT_MAX;
        int bestSq  = -1;

        int toRank = toIdx >> 3;
        int toFile = toIdx & 7;

        // Pawns
        {
            Piece pawn = black ? BLACK_PAWN : WHITE_PAWN;
            int dr = black ? -1 : 1; // direction from pawn to capture square
            for (int df : {-1, 1}) {
                int r = toRank + dr;
                int f = toFile + df;
                if (r < 0 || r >= 8 || f < 0 || f >= 8) continue;
                int sq = r * 8 + f;
                if ((occ >> sq & 1) && board.getPiece(Position(uint8_t(sq))) == pawn) {
                    int value = seeValue(pawn);
                    if (value < bestVal) { bestVal = value; bestSq = sq; }
                }
            }
        }

        // Knights
        {
            Piece knight = black ? BLACK_KNIGHT : WHITE_KNIGHT;
            for (auto& o : knightOffsets) {
                int r = toRank + o[0];
                int f = toFile + o[1];
                if (r < 0 || r >= 8 || f < 0 || f >= 8) continue;
                int sq = r * 8 + f;
                if ((occ >> sq & 1) && board.getPiece(Position(uint8_t(sq))) == knight) {
                    int value = seeValue(knight);
                    if (value < bestVal) { bestVal = value; bestSq = sq; }
                }
            }
        }

        // Bishops / Queens (diagonal)
        {
            Piece bishop = black ? BLACK_BISHOP : WHITE_BISHOP;
            Piece queen  = black ? BLACK_QUEEN  : WHITE_QUEEN;
            for (auto& dd : diagDirs) {
                int r = toRank, f = toFile;
                for (int step = 1; step < 8; step++) {
                    r += dd[0]; f += dd[1];
                    if (r < 0 || r >= 8 || f < 0 || f >= 8) break;
                    int sq = r * 8 + f;
                    if (!(occ >> sq & 1)) continue;
                    Piece p = board.getPiece(Position(uint8_t(sq)));
                    if (p == bishop || p == queen) {
                        int value = seeValue(p);
                        if (value < bestVal) { bestVal = value; bestSq = sq; }
                    }
                    break;
                }
            }
        }

        // Rooks / Queens (straight)
        {
            Piece rook  = black ? BLACK_ROOK  : WHITE_ROOK;
            Piece queen = black ? BLACK_QUEEN : WHITE_QUEEN;
            for (auto& sd : straightDirs) {
                int r = toRank, f = toFile;
                for (int step = 1; step < 8; step++) {
                    r += sd[0]; f += sd[1];
                    if (r < 0 || r >= 8 || f < 0 || f >= 8) break;
                    int sq = r * 8 + f;
                    if (!(occ >> sq & 1)) continue;
                    Piece p = board.getPiece(Position(uint8_t(sq)));
                    if (p == rook || p == queen) {
                        int value = seeValue(p);
                        if (value < bestVal) { bestVal = value; bestSq = sq; }
                    }
                    break;
                }
            }
        }

        // King
        {
            Piece king = black ? BLACK_KING : WHITE_KING;
            for (auto& o : kingOffsets) {
                int r = toRank + o[0];
                int f = toFile + o[1];
                if (r < 0 || r >= 8 || f < 0 || f >= 8) continue;
                int sq = r * 8 + f;
                if ((occ >> sq & 1) && board.getPiece(Position(uint8_t(sq))) == king) {
                    int value = seeValue(king);
                    if (value < bestVal) { bestVal = value; bestSq = sq; }
                }
            }
        }

        return {bestSq == -1 ? -1 : bestVal, bestSq};
    };

    // Simulate exchanges
    Piece currentAttacker = attacker;
    d = 0;
    gain[d] = seeValue(victim);

    while (true) {
        if (d >= 31) break;
        d++;
        sideToMove = !sideToMove;
        auto [lvaVal, lvaSq] = leastValuableAttacker(sideToMove);
        if (lvaSq == -1) break;

        // The capturing piece is now at lvaSq; remove it from occ
        occ &= ~(1ULL << lvaSq);

        // After capturing, the gain is "capture value of current piece" minus what opponent gets
        gain[d] = seeValue(currentAttacker) - gain[d-1];
        currentAttacker = board.getPiece(Position(uint8_t(lvaSq)));

        if (std::max(-gain[d-1], gain[d]) < 0) break; // prune — both sides lose
    }

    // Negate back to get result from original side's perspective
    while (--d > 0) {
        gain[d-1] = -std::max(-gain[d-1], gain[d]);
    }
    return gain[0];
}


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
        Board    board;
        Move     killerMoves[MAX_DEPTH][2]{};
        int      history[64][64]{};
        PVTable  pv{};              // PV table for this thread
    };

    void runSearch() {
        MoveList moves;
        board.getValidMoves(moves);

        if (moves.count == 0) {
            finish();
            return;
        }

        std::vector<std::thread> workers;
        std::vector<ThreadData>  threadData(threadCount);

        for (int t = 0; t < threadCount; t++) {
            threadData[t].board.set(board);
        }

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

        Move  bestMove = moves[0];
        float bestEval = -SEARCH_INF;

        int startDepth = 1 + (threadIndex % 4);

        // Aspiration window state
        float prevScore  = 0.0f;
        bool  firstDepth = true;

        for (int depth = startDepth; depth <= MAX_DEPTH; depth++) {

            // Clear PV for this iteration so stale lines don't bleed in
            td.pv.clear();

            float alpha, beta;

            if (firstDepth || depth < 4) {
                alpha = -SEARCH_INF;
                beta  =  SEARCH_INF;
            } else {
                // Aspiration window: start tight, widen on fail
                constexpr float WINDOW = 0.50f; // ~50 centipawn equivalent
                alpha = prevScore - WINDOW;
                beta  = prevScore + WINDOW;
            }

            Move  currentBestMove = bestMove;
            float currentBestEval = -SEARCH_INF;

            // Aspiration retry loop
            while (true) {
                float searchAlpha = alpha;
                float searchBeta  = beta;

                // Use PV move from previous iteration for root move ordering
                Move pvMove = td.pv.hasPVMove(0) ? td.pv.pvMove(0) : bestMove;
                sortMoves(td.board, moves, pvMove, td.killerMoves[0], td.history);

                currentBestEval = -SEARCH_INF;
                float localAlpha = searchAlpha;

                for (int i = 0; i < moves.count; i++) {
                    if (abort.load()) return;

                    const Move& move = moves[i];
                    td.board.move(move);
                    float eval = -search(td.board, depth - 1, -searchBeta, -localAlpha,
                                         1, td.killerMoves, td.history, td.pv);
                    td.board.undoMove();

                    if (abort.load()) return;

                    if (eval > currentBestEval) {
                        currentBestEval = eval;
                        currentBestMove = move;
                        // Update root PV when we find a new best move
                        td.pv.updatePV(0, move);
                    }
                    if (eval > localAlpha) localAlpha = eval;
                    if (localAlpha >= searchBeta) break;
                }

                // Aspiration window management
                if (currentBestEval <= searchAlpha || currentBestEval >= searchBeta) {
                    if (searchAlpha <= -SEARCH_INF && searchBeta >= SEARCH_INF) {
                        break;
                    }

                    alpha = -SEARCH_INF;
                    beta  =  SEARCH_INF;
                } else {
                    break; // inside window — done
                }
            }

            bestMove    = currentBestMove;
            bestEval    = currentBestEval;
            prevScore   = bestEval;
            firstDepth  = false;

            if (threadIndex == 0) {
                updateResult(bestMove, bestEval, depth, false);
            }
        }
    }

    // -----------------------------------------------------------------------
    // Main alpha-beta search with:
    //   - Transposition table
    //   - PV move ordering (from previous iteration's PV)
    //   - Null move pruning
    //   - Late Move Reductions (LMR)
    //   - Killer / history move ordering
    //   - SEE-based capture ordering
    // -----------------------------------------------------------------------
    float search(Board& board, int depth, float alpha, float beta, int ply,
                 Move killerMoves[MAX_DEPTH][2], int history[64][64], PVTable& pv)
    {
        if (shouldAbort()) {
            abort = true;
            return alpha;
        }

        // Reset child PV length so stale data doesn't propagate
        if (ply + 1 < MAX_DEPTH) pv.pvLength[ply + 1] = 0;

        if (ply >= MAX_DEPTH - 1) {
            incrementPositions();
            return evaluateBoard(board);
        }

        if (depth <= 0) {
            return searchCaptures(board, alpha, beta, ply, killerMoves, history);
        }

        if (board.draw50Move() || board.repetition()) {
            return 0.0f;
        }

        float originalAlpha = alpha;
        float originalBeta  = beta;
        uint64_t hash = board.getHash();

        // --- Transposition table probe ---
        Move ttMove{};
        bool hasTTMove = false;

        Entry entry = tt.find(hash);
        if (entry.flag != FAIL && entry.hash == hash) {
            hasTTMove = true;
            ttMove    = entry.bestMove;
            float ttEval = scoreFromTT(entry.eval, ply);

            if (entry.depth >= depth) {
                incrementTTLookups();
                if      (entry.flag == EXACT)      return ttEval;
                else if (entry.flag == LOWERBOUND)  alpha = std::max(alpha, ttEval);
                else if (entry.flag == UPPERBOUND)  beta  = std::min(beta,  ttEval);
                if (alpha >= beta) return ttEval;
            }
        }

        bool inCheck = board.check(board.getPlayerTurn());

        // --- Null move pruning ---
        if (!inCheck && depth >= 3 && ply > 0 && beta < SEARCH_MATE_SCORE - 1000) {
            int phase = board.getEvalPhase();
            bool isEndgame = phase <= 4;

            if (!isEndgame) {
                int R = 3 + depth / 6;
                board.makeNullMove();
                float nullScore = -search(board, depth - 1 - R, -beta, -beta + 1.0f,
                                          ply + 1, killerMoves, history, pv);
                board.undoNullMove();

                if (abort) return alpha;

                if (nullScore >= beta) {
                    return beta;
                }
            }
        }

        // --- Generate and sort moves ---
        MoveList moves;
        board.getValidMoves(moves);

        if (moves.count == 0) {
            if (inCheck) return -SEARCH_MATE_SCORE + float(ply);
            return 0.0f; // stalemate
        }

        // Check extension
        int extension = inCheck ? 1 : 0;
        int plyIndex  = std::min(ply, MAX_DEPTH - 1);

        // Priority for ordering: TT move > PV move > killers > history
        // We pass the TT move to sortMoves (highest priority slot).
        // The PV move is injected as a secondary hint — if no TT move exists,
        // the PV move from the previous iteration fills that slot instead.
        Move orderingMove = hasTTMove ? ttMove : pv.pvMove(ply);
        sortMoves(board, moves, orderingMove, killerMoves[plyIndex], history);

        Move  bestMove = moves[0];
        float bestEval = -SEARCH_INF;

        // --- Futility pruning ---
        // At low depths, if static eval is so far below alpha that even a large
        // material swing can't save it, skip quiet moves entirely.
        // Margins: depth 1 ~= minor piece (325), depth 2 ~= rook (500).
        // Never prune when in check, near mate, or at the root (ply 0).
        bool canFutilityPrune = false;
        float futilityBase    = 0.0f;
        constexpr float FUTILITY_MARGIN[3] = { 0.0f, 3.25f, 5.00f }; // indexed by depth

        if (!inCheck && depth <= 2 && ply > 0
                && alpha < SEARCH_MATE_SCORE - 1000
                && beta  < SEARCH_MATE_SCORE - 1000)
        {
            futilityBase     = evaluateBoard(board) + FUTILITY_MARGIN[depth];
            canFutilityPrune = (futilityBase <= alpha);
        }

        for (int i = 0; i < moves.count; i++) {
            Move move = moves[i];
            int  newDepth = depth - 1 + extension;
            float eval;

            // Apply futility pruning: skip quiet, non-killer moves that can't raise alpha
            if (canFutilityPrune
                    && i > 0                       // never skip the first (TT/PV) move
                    && !move.isCapture()
                    && !move.isPromotion()
                    && !(move == killerMoves[plyIndex][0])
                    && !(move == killerMoves[plyIndex][1]))
            {
                continue;
            }

            board.move(move);

            bool givesCheck = board.check(board.getPlayerTurn());

            // --- Late Move Reductions ---
            // Reduce more aggressively: start at move 2 (not 3), divide by 2.0 (not 3.0)
            bool doLMR = (
                i >= 2              &&
                depth >= 3          &&
                !move.isCapture()   &&
                !move.isPromotion() &&
                !inCheck            &&
                !givesCheck
            );

            if (doLMR) {
                int reduction = 1 + int(std::log(float(depth)) * std::log(float(i)) / 2.0f);
                reduction = std::min(reduction, depth - 2);

                eval = -search(board, newDepth - reduction, -alpha - 1.0f, -alpha,
                               ply + 1, killerMoves, history, pv);

                if (!abort && eval > alpha) {
                    eval = -search(board, newDepth, -alpha - 1.0f, -alpha,
                                   ply + 1, killerMoves, history, pv);
                }

                if (!abort && eval > alpha && eval < beta) {
                    eval = -search(board, newDepth, -beta, -alpha,
                                   ply + 1, killerMoves, history, pv);
                }
            } else if (i == 0) {
                eval = -search(board, newDepth, -beta, -alpha,
                               ply + 1, killerMoves, history, pv);
            } else {
                eval = -search(board, newDepth, -alpha - 1.0f, -alpha,
                               ply + 1, killerMoves, history, pv);

                if (!abort && eval > alpha && eval < beta) {
                    eval = -search(board, newDepth, -beta, -alpha,
                                   ply + 1, killerMoves, history, pv);
                }
            }

            board.undoMove();

            if (abort) return alpha;

            if (eval > bestEval) {
                bestEval = eval;
                bestMove = move;
                // New best: record this move in the PV and copy child's line up
                pv.updatePV(ply, move);
            }

            alpha = std::max(alpha, eval);

            if (alpha >= beta) {
                // Beta cutoff — update killer and history for quiet moves
                if (!move.isCapture()) {
                    if (!(move == killerMoves[plyIndex][0])) {
                        killerMoves[plyIndex][1] = killerMoves[plyIndex][0];
                        killerMoves[plyIndex][0] = move;
                    }
                    history[move.starting().index][move.target().index] += depth * depth;
                }
                break;
            }
        }

        // --- Store in TT ---
        if (!abort) {
            Entry newEntry{};
            newEntry.hash     = hash;
            newEntry.eval     = scoreToTT(bestEval, ply);
            newEntry.depth    = int8_t(depth);
            newEntry.bestMove = bestMove;

            if      (bestEval <= originalAlpha) newEntry.flag = UPPERBOUND;
            else if (bestEval >= originalBeta)  newEntry.flag = LOWERBOUND;
            else                                newEntry.flag = EXACT;

            tt.store(newEntry);
        }

        return bestEval;
    }

    float searchCaptures(Board& board, float alpha, float beta, int ply,
                         Move killerMoves[MAX_DEPTH][2], int history[64][64])
    {
        if (shouldAbort()) {
            abort = true;
            return alpha;
        }

        if (ply >= MAX_DEPTH - 1) {
            incrementPositions();
            return evaluateBoard(board);
        }

        if (board.draw50Move() || board.repetition()) {
            return 0.0f;
        }

        float standPat = evaluateBoard(board);
        incrementPositions();

        if (standPat >= beta)  return standPat;
        if (standPat > alpha)  alpha = standPat;

        MoveList moves;
        board.getCaptureMoves(moves);
        sortMoves(board, moves, Move{}, killerMoves[std::min(ply, MAX_DEPTH-1)], history);

        for (int i = 0; i < moves.count; i++) {
            Move move = moves[i];

            if (!move.isPromotion()) {
                Piece victim = board.getPiece(move.target());
                if (standPat + float(seeValue(victim)) + 200.0f < alpha) {
                    continue;
                }
            }

            board.move(move);
            float eval = -searchCaptures(board, -beta, -alpha, ply + 1, killerMoves, history);
            board.undoMove();

            if (abort) return alpha;

            if (eval >= beta)   return beta;
            if (eval > alpha)   alpha = eval;
        }

        return alpha;
    }

    // -----------------------------------------------------------------------
    // Move scoring for ordering
    // Priority (descending):
    //   TT/PV move > promotions > winning captures (SEE>=0) > killers >
    //   history quiet moves > losing captures (SEE<0)
    // -----------------------------------------------------------------------
    [[nodiscard]] static float guessMoveScore(Board& board, const Move& move,
                                              const Move& ttMove,
                                              const Move killerMoves[2],
                                              const int history[64][64])
    {
        // TT move OR PV move — both arrive via the same 'ttMove' slot
        if (move == ttMove) return 10'000'000.0f;

        float score = 0.0f;

        // Promotions
        if (move.isPromotion()) {
            switch (move.promotionPiece()) {
                case 0:  score += 9'000'000.0f; break; // Queen
                case 1:  score += 5'000'000.0f; break; // Rook
                default: score += 3'000'000.0f; break; // Bishop/Knight
            }
        }

        // Captures — score by SEE to put winning captures first, losing last
        if (move.isCapture()) {
            int seeScore = see(board, move);

            if (seeScore >= 0) {
                score += 1'000'000.0f + float(seeScore);
            } else {
                score += float(seeScore) - 100'000.0f;
            }
        } else {
            // Quiet moves
            if (move == killerMoves[0]) {
                score += 800'000.0f;
            } else if (move == killerMoves[1]) {
                score += 700'000.0f;
            } else {
                score += float(history[move.starting().index][move.target().index]);
            }
        }

        return score;
    }

    static void sortMoves(Board& board, MoveList& moves, const Move& ttMove,
                          const Move killerMoves[2], const int history[64][64])
    {
        std::array<ScoredMove, 256> scoredMoves;

        for (int i = 0; i < moves.count; i++) {
            scoredMoves[i] = {
                moves[i],
                guessMoveScore(board, moves[i], ttMove, killerMoves, history)
            };
        }

        std::sort(scoredMoves.begin(), scoredMoves.begin() + moves.count,
                  [](const ScoredMove& a, const ScoredMove& b) {
                      return a.score > b.score;
                  });

        for (int i = 0; i < moves.count; i++) {
            moves[i] = scoredMoves[i].move;
        }
    }

    bool shouldAbort() {
        int n = nodesSinceTimeCheck.fetch_add(1, std::memory_order_relaxed);
        if ((n & 4095) != 0) return false;
        return abort.load(std::memory_order_relaxed);
    }

    static int averageNodesPerSecond(int nodes, int milliseconds) {
        if (milliseconds <= 0) return 0;
        return int(std::round(double(nodes) * 1000.0 / double(milliseconds)));
    }

    void updateResult(const Move& bestMove, float bestEval, int depth, bool finished) {
        std::lock_guard lock(resultMutex);

        result.bestMove = bestMove;
        result.depth    = depth;
        result.finished = finished;

        result.positionsEvaluated = positionsEvaluated.load();
        result.timeTaken =
            int(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime
            ).count());
        result.nodesPerSecond =
            averageNodesPerSecond(result.positionsEvaluated, result.timeTaken);
        result.ttLookups          = ttLookups.load();

        result.mate = std::abs(bestEval) > SEARCH_MATE_SCORE - 1000;

        if (result.mate) {
            int matePlies = std::max(1, int(std::round(SEARCH_MATE_SCORE - std::abs(bestEval))));
            int mateMoves = bestEval > 0.0f
                ? (matePlies + 1) / 2
                : matePlies / 2;

            result.eval = (bestEval < 0.0f ? -1.0f : 1.0f) * float(std::max(1, mateMoves));
        } else {
            result.eval = bestEval;
        }
    }

    void finish() {
        {
            std::lock_guard lock(resultMutex);
            result.positionsEvaluated = positionsEvaluated.load();
            result.ttLookups          = ttLookups.load();
            result.finished           = true;
            result.timeTaken =
                int(std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - startTime
                ).count());
            result.nodesPerSecond =
                averageNodesPerSecond(result.positionsEvaluated, result.timeTaken);
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
