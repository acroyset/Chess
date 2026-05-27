#ifndef EVALUTATION_H
#define EVALUTATION_H

#include <algorithm>
#include "../Board.h"
#include "../offsets.h"

// -----------------------------------------------------------------------
// Side data: pre-collected piece positions, pawn structure info, etc.
// -----------------------------------------------------------------------
struct EvalSideData {
    int pawnFiles[8]{};
    uint8_t pawnFileMask = 0;
    int bishops = 0;

    Position kingPos{};
    bool hasKing = false;

    Position pawns[8];
    int pawnCount = 0;

    int pawnMinRank[8]{};
    int pawnMaxRank[8]{};

    Position knights[12];
    int knightCount = 0;

    Position bishopSquares[12];

    Position rooks[12];
    int rookCount = 0;

    Position queens[12];
    int queenCount = 0;

    EvalSideData() {
        for (int f = 0; f < 8; f++) { pawnMinRank[f] = 8; pawnMaxRank[f] = -1; }
    }
};

class Evaluator {

public:

    static float evaluate(const Board& board);
    static int see(const Board& board, Move move);

private:

    [[nodiscard]] static Piece evalPieceAt(const Board& board, int rank, int file);

    // Fast bitboard-based piece lookup (prefer over evalPieceAt in hot loops)
    [[nodiscard]] static Piece evalPieceAtBB(const Board& board, int sq);

    [[nodiscard]] static float evalMobilityCached(const Board& board, const EvalSideData& white, const EvalSideData& black, float midgameT, float endgameT);

    // -----------------------------------------------------------------------
    // King attack evaluation
    // FIX: countSlidingKingZoneAttacks now correctly stops the ray at the
    // first blocking piece — previously it counted zone squares even when
    // the ray was physically blocked by an intervening piece.
    // -----------------------------------------------------------------------


    template <size_t N>
    [[nodiscard]] static int countSlidingKingZoneAttacks(const Board& board, Position from, const int (&directions)[N][2], uint64_t zone);

    [[nodiscard]] static float evalKingSafety(const Board& board, const EvalSideData& defender, const EvalSideData& attacker, bool defenderBlack, float midgameT);

    // -----------------------------------------------------------------------
    // King shield
    // -----------------------------------------------------------------------
    [[nodiscard]] static float evalKingShieldCached(const Board& board, const EvalSideData& side, bool black);

    // -----------------------------------------------------------------------
    // Center control
    // -----------------------------------------------------------------------
    [[nodiscard]] static float evalCenterCached(const Board& board, bool black);

    // -----------------------------------------------------------------------
    // Trapped pieces — piece has no safe moves (all exits attacked by
    // enemy pawns or lead off board). Distinct from mobility: a piece can
    // have legal moves but all of them lose material.
    // -----------------------------------------------------------------------
    [[nodiscard]] static float evalTrappedPieces(const Board& board, const EvalSideData& side, const EvalSideData& enemy, bool black, float midgameT);

    // -----------------------------------------------------------------------
    // Castling status
    // FIX: reduced the castled-king bonus from 0.38 to 0.18 and raised the
    // castling-rights bonus to 0.12/0.10 so the eval cliff when castling
    // occurs is much smaller. The pawn-shield eval already rewards castled
    // kings; the old bonus on top caused the engine to overvalue castling
    // and misread positions where castling wasn't the critical factor.
    // -----------------------------------------------------------------------
    [[nodiscard]] static float evalCastlingStatus(const Board& board, const EvalSideData& side, bool black);

    // -----------------------------------------------------------------------
    // Threats — only count attacks that are genuinely dangerous:
    //   - Attacks on undefended pieces (free)
    //   - Forks (piece attacks 2+ enemy pieces)
    //   - Attacks where SEE > 0 (winning exchange)
    // Does NOT reward attacking a well-defended piece with no gain.
    // -----------------------------------------------------------------------
    [[nodiscard]] static float evalThreats(const Board& board, const EvalSideData& us, const EvalSideData& them, bool usBlack, uint64_t enemyAttacks);

    // -----------------------------------------------------------------------
    // Attack mask builders — use already-collected piece positions
    // -----------------------------------------------------------------------
    [[nodiscard]] static uint64_t buildAttackMask(const Board& board, const EvalSideData& side, bool black);

};

#endif