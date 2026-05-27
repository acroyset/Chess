#ifndef EVALUTATION_H
#define EVALUTATION_H

#include "../Game/Board.h"

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

struct AttackData {
    uint64_t mask = 0;          // full attack mask (used by piece safety + threats)
    float    mobilityMg = 0.0f; // weighted mobility score, midgame
    float    mobilityEg = 0.0f; // weighted mobility score, endgame
    int      kingZoneHits[6]{}; // hits per piece type (indexed 1-5) into enemy king zone
    int      kingZonePieces = 0;// number of distinct piece types hitting the zone
};

class Evaluator {

public:

    static float evaluate(const Board& board);
    static int see(const Board& board, Move move);

private:

    [[nodiscard]] static Piece evalPieceAt(const Board& board, int rank, int file);

    // Fast bitboard-based piece lookup (prefer over evalPieceAt in hot loops)
    [[nodiscard]] static Piece evalPieceAtBB(const Board& board, int sq);

    // -----------------------------------------------------------------------
    // King attack evaluation
    // FIX: countSlidingKingZoneAttacks now correctly stops the ray at the
    // first blocking piece — previously it counted zone squares even when
    // the ray was physically blocked by an intervening piece.
    // -----------------------------------------------------------------------

    [[nodiscard]] static float evalKingSafety(const EvalSideData& defender, const EvalSideData& attacker, bool defenderBlack, float midgameT, const AttackData& attackerData);

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
    [[nodiscard]] static float evalTrappedPieces(const Board& board, const EvalSideData& side, const EvalSideData& enemy, bool black, float midgameT, uint64_t enemyAllAttacks);

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

    [[nodiscard]] static AttackData buildAttackData(const Board& board, const EvalSideData& side, bool black, uint64_t enemyKingZone);

    // -----------------------------------------------------------------------
    // King Pawns Attacked
    // -----------------------------------------------------------------------
    [[nodiscard]] static int countPawnKingZoneAttacks(Position from, bool black, uint64_t zone);

    // -----------------------------------------------------------------------
    // Mobility
    // -----------------------------------------------------------------------
    [[nodiscard]] static float knightMobility(Position from, uint64_t ownPieces) ;

    [[nodiscard]] static float slidingMobility(Position from, uint64_t occ, uint64_t ownPieces, const int dirs[][2], int numDirs);

    // -----------------------------------------------------------------------
    // Pawn structure
    // -----------------------------------------------------------------------
    [[nodiscard]] static float evalPawnStructureCached(const EvalSideData& side, const EvalSideData& enemy, bool black, float endgameT);

    // -----------------------------------------------------------------------
    // Rooks: open/half-open files, seventh rank
    // -----------------------------------------------------------------------
    [[nodiscard]] static float evalRooksCached(const EvalSideData& side, const EvalSideData& enemy, bool black);
    [[nodiscard]] static float rookBehindPassedPawn(const EvalSideData& side, const EvalSideData& enemy, bool black);

    // -----------------------------------------------------------------------
    // Space evaluation
    // -----------------------------------------------------------------------
    [[nodiscard]] static float evalSpace(const EvalSideData& side, bool black);

    // -----------------------------------------------------------------------
    // Outpost detection
    // -----------------------------------------------------------------------
    [[nodiscard]] static bool isOutpostSquare(int rank, int file, const EvalSideData& enemy, bool pieceIsBlack);

    [[nodiscard]] static bool isSupportedByPawn(int rank, int file, const EvalSideData& side, bool pieceIsBlack);

    [[nodiscard]] static float evalOutposts(const EvalSideData& side, const EvalSideData& enemy, bool black, float midgameT, uint64_t enemyAttacks);

    // SEE piece values (centipawns-ish, integers for speed)
    [[nodiscard]] static int seeValue(Piece piece);

    // -----------------------------------------------------------------------
    // Insufficient material / draw heuristics
    // -----------------------------------------------------------------------
    [[nodiscard]] static float drawScaleFactor(const EvalSideData& white, const EvalSideData& black);

    // -----------------------------------------------------------------------
    // Endgame king activity
    // -----------------------------------------------------------------------
    [[nodiscard]] static float endgameKingActivity(Position king);

    [[nodiscard]] static float kingSupportsPawns(const EvalSideData& side, const EvalSideData& enemy, bool black);

    // -----------------------------------------------------------------------
    // Mop-up
    // -----------------------------------------------------------------------
    [[nodiscard]] static float mopUpEval(const EvalSideData& winning, const EvalSideData& losing, float advantage);

    // -----------------------------------------------------------------------
    // Pawn-king race
    // -----------------------------------------------------------------------
    [[nodiscard]] static float pawnKingRaceBonus(const EvalSideData& side, const EvalSideData& enemy, bool black);

    // -----------------------------------------------------------------------
    // Piece safety — now O(pieces) bitwise lookups, no ray scans.
    // -----------------------------------------------------------------------
    [[nodiscard]] static float evalPieceSafetyCached(const EvalSideData& side, bool black, float endgameT, uint64_t friendlyAttacks, uint64_t enemyAttacks);

};

#endif