//
// Created by Andreas Royset on 5/22/26.
//

#include "Searcher.h"

float scoreToTT(float score, int ply) {
    if (score > SEARCH_MATE_SCORE - 1000)  return score + float(ply);
    if (score < -SEARCH_MATE_SCORE + 1000) return score - float(ply);
    return score;
}
float scoreFromTT(float score, int ply) {
    if (score > SEARCH_MATE_SCORE - 1000)  return score - float(ply);
    if (score < -SEARCH_MATE_SCORE + 1000) return score + float(ply);
    return score;
}

void Searcher::startSearch(const Board& startBoard) {
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
SearchResult Searcher::endSearch() {
    abort = true;

    if (thread.joinable()) {
	    thread.join();
    }

    searching = false;

    std::lock_guard lock(resultMutex);
    return result;
}


void Searcher::runSearch() {
    if (board.draw50Move() || board.drawRepetition() || board.drawInsufficientMaterial() || board.checkMate()) {
	    finish();
	    return;
    }

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

void Searcher::searchThread(ThreadData& td, int threadIndex) {
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

        if (depth > 1) {
            for (auto & a : td.history)
                for (int & b : a)
                    b /= 2;
        }

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

                bool givesCheck = td.board.check(td.board.getPlayerTurn());
                int rootDepth = depth - 1 + (givesCheck ? 1 : 0);

                float eval = -search(td.board, rootDepth, -searchBeta, -localAlpha,
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
                // Already at full window — nothing left to try
                if (searchAlpha <= -SEARCH_INF && searchBeta >= SEARCH_INF) {
                    break;
                }
                // Open window fully; if it's a mate skip narrowing next iter
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

float Searcher::search(Board& board, int depth, float alpha, float beta, int ply, Move killerMoves[MAX_DEPTH][2], int history[64][64], PVTable& pv) {
    if (shouldAbort()) {
        abort = true;
        return alpha;
    }

    // Reset child PV length so stale data doesn't propagate
    if (ply + 1 < MAX_DEPTH) pv.pvLength[ply + 1] = 0;

    if (ply >= MAX_DEPTH - 1) {
        incrementPositions();
        return Evaluator::evaluate(board);
    }

    if (depth <= 0) {
        return searchCaptures(board, alpha, beta, ply, killerMoves, history);
    }

    if (board.draw50Move() || board.drawRepetition() || board.drawInsufficientMaterial()) {
        return 0.0f;
    }

    float mateAlpha = -SEARCH_MATE_SCORE + float(ply);
    float mateBeta  =  SEARCH_MATE_SCORE - float(ply + 1);
    if (mateAlpha > alpha) alpha = mateAlpha;
    if (mateBeta  < beta)  beta  = mateBeta;
    if (alpha >= beta)     return alpha;

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
            if (entry.flag == LOWERBOUND)  alpha = std::max(alpha, ttEval);
            else if (entry.flag == UPPERBOUND)  beta  = std::min(beta,  ttEval);
            if (alpha >= beta) return ttEval;
        }
    }

    bool inCheck = board.check(board.getPlayerTurn());

    // --- Null move pruning ---
    if (!inCheck && depth >= 3 && ply > 0 &&
        beta < SEARCH_MATE_SCORE - 1000 &&
        alpha > -(SEARCH_MATE_SCORE - 1000))
        {
        int phase = board.getEvalPhase();
        bool isEndgame = phase <= 4;

        if (!isEndgame) {
            int R = 3 + depth / 6;
            board.makeNullMove();
            float nullScore = -search(board, depth - 1 - R, -beta, -beta + 1.0f,
                                      ply + 1, killerMoves, history, pv);
            board.undoMove();

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
    constexpr float FUTILITY_MARGIN[3] = { 0.0f, 2.00f, 3.50f };

    if (!inCheck && depth <= 2 && ply > 0
            && alpha < SEARCH_MATE_SCORE - 1000
            && beta  < SEARCH_MATE_SCORE - 1000)
    {
        futilityBase     = Evaluator::evaluate(board) + FUTILITY_MARGIN[depth];
        canFutilityPrune = (futilityBase <= alpha);
    }

    for (int i = 0; i < moves.count; i++) {
        Move move = moves[i];

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

        // Detect if this move gives check to the opponent
        bool givesCheck = board.moveGivesCheck(move);

        int newDepth = depth - 1;

        // Extend for: being in check before move, giving check, captures of high value pieces
        if (inCheck || givesCheck) {
            newDepth++; // escaping check, giving check: don't reduce
        }

        // LMR: never reduce if we give check, are in check, capture, or promote
        bool doLMR = (
            i >= 2              &&
            depth >= 3          &&
            !move.isCapture()   &&
            !move.isPromotion() &&
            !inCheck            &&
            !givesCheck
        );

        if (doLMR) {
            int reduction = 1 + int(std::log(float(depth)) * std::log(float(i)) / 3.0f);
            reduction = std::min(reduction, depth - 2); // never reduce to 0

            // Don't reduce into qsearch directly
            int reducedDepth = std::max(1, newDepth - reduction);

            eval = -search(board, reducedDepth, -alpha - 1.0f, -alpha,
                           ply + 1, killerMoves, history, pv);

            // Re-search at full depth without reduction if it beat alpha
            if (!abort && eval > alpha) {
                eval = -search(board, newDepth, -alpha - 1.0f, -alpha,
                               ply + 1, killerMoves, history, pv);
            }

            // Full window re-search if it still beats alpha
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

float Searcher::searchCaptures(Board& board, float alpha, float beta, int ply, Move killerMoves[MAX_DEPTH][2], int history[64][64]) {
    if (shouldAbort()) { abort = true; return alpha; }
    if (ply >= MAX_DEPTH - 1) { incrementPositions(); return Evaluator::evaluate(board); }
    if (board.draw50Move() || board.drawRepetition() || board.drawInsufficientMaterial()) return 0.0f;

    bool inCheck = board.check(board.getPlayerTurn());

    if (inCheck) {
        // In check: must find a legal move — generate ALL moves, not just captures
        // Can't stand pat when in check
        MoveList moves;
        board.getValidMoves(moves);

        if (moves.count == 0) return -SEARCH_MATE_SCORE + float(ply); // checkmate

        // Sort: try captures first, then quiets
        sortMoves(board, moves, Move{}, killerMoves[std::min(ply, MAX_DEPTH-1)], history);

        float bestEval = -SEARCH_INF;
        for (int i = 0; i < moves.count; i++) {
            board.move(moves[i]);
            float eval = -searchCaptures(board, -beta, -alpha, ply + 1, killerMoves, history);
            board.undoMove();
            if (abort) return alpha;
            if (eval > bestEval) bestEval = eval;
            if (eval > alpha)    alpha = eval;
            if (alpha >= beta)   return beta;
        }
        return bestEval;
    }

    // Not in check: normal stand-pat
    float standPat = Evaluator::evaluate(board);
    incrementPositions();

    if (standPat >= beta)  return standPat;
    if (standPat > alpha)  alpha = standPat;

    MoveList moves;
    board.getCaptureMoves(moves);
    sortMoves(board, moves, Move{}, killerMoves[std::min(ply, MAX_DEPTH-1)], history);

    for (int i = 0; i < moves.count; i++) {
        Move move = moves[i];

        // SEE filter: skip clearly losing captures UNLESS they give check
        // because a losing capture that gives check might win material next move
        board.move(move);
        bool movGivesCheck = board.check(board.getPlayerTurn());
        board.undoMove();

        if (!movGivesCheck && !move.isPromotion()) {
            int seeScore = Evaluator::see(board, move);

            // Losing captures are usually not worth searching in qsearch.
            if (seeScore < 0) {
                continue;
            }

            // Delta pruning: if even the SEE gain plus margin cannot raise alpha, skip.
            if (standPat + float(seeScore) + 150.0f < alpha) {
                continue;
            }
        }

        board.move(move);
        float eval = -searchCaptures(board, -beta, -alpha, ply + 1, killerMoves, history);
        board.undoMove();

        if (abort) return alpha;
        if (eval >= beta)  return beta;
        if (eval > alpha)  alpha = eval;
    }

    return alpha;
}

[[nodiscard]] float Searcher::guessMoveScore(const Board& board, const Move& move, const Move& ttMove, const Move killerMoves[2], const int history[64][64]){
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
        int seeScore = Evaluator::see(board, move);

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

void Searcher::sortMoves(Board& board, MoveList& moves, const Move& ttMove, const Move killerMoves[2], const int history[64][64]) {
    // Score all moves first
    static thread_local std::array<float, 256> scores;
    for (int i = 0; i < moves.count; i++)
        scores[i] = guessMoveScore(board, moves[i], ttMove, killerMoves, history);

    // Insertion sort — faster than std::sort for typical move counts (20-40)
    for (int i = 1; i < moves.count; i++) {
        Move  m = moves[i];
        float s = scores[i];
        int j = i - 1;
        while (j >= 0 && scores[j] < s) {
            moves[j+1]  = moves[j];
            scores[j+1] = scores[j];
            j--;
        }
        moves[j+1]  = m;
        scores[j+1] = s;
    }
}

void Searcher::updateResult(const Move& bestMove, float bestEval, int depth, bool finished) {
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

void Searcher::finish() {
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

