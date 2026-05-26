#ifndef EVALUTATION_H
#define EVALUTATION_H

#include <cmath>
#include <algorithm>
#include "../Board.h"

constexpr float INF_EVAL = 1000000.0f;

// -----------------------------------------------------------------------
// Piece-square tables (PST) — from white's perspective, rank 0 = white's
// back rank. Black pieces mirror via XOR 56.
// Values are *bonuses* added on top of base piece values in BAKED tables.
// -----------------------------------------------------------------------

constexpr float PAWN_MG[64] = {
     0,    0,    0,    0,    0,    0,    0,    0,
     .60,  .60,  .60,  .40,  .40,  .60,  .60,  .60,
     .14,  .14,  .24,  .34,  .34,  .24,  .14,  .14,
     .06,  .08,  .14,  .30,  .30,  .14,  .08,  .06,
     .02,  .04,  .08,  .24,  .24,  .08,  .04,  .02,
     .04, -.06, -.10,  .02,  .02, -.10, -.06,  .04,
     .04,  .10,  .10, -.18, -.18,  .10,  .10,  .04,
     0,    0,    0,    0,    0,    0,    0,    0
};

constexpr float PAWN_EG[64] = {
     0,    0,    0,    0,    0,    0,    0,    0,
     .90,  .90,  .90,  .90,  .90,  .90,  .90,  .90,
     .55,  .55,  .58,  .65,  .65,  .58,  .55,  .55,
     .28,  .28,  .34,  .44,  .44,  .34,  .28,  .28,
     .14,  .14,  .20,  .30,  .30,  .20,  .14,  .14,
     .06,  .06,  .08,  .12,  .12,  .08,  .06,  .06,
     .02,  .02,  .02, -.04, -.04,  .02,  .02,  .02,
     0,    0,    0,    0,    0,    0,    0,    0
};

constexpr float KNIGHT_MG[64] = {
    -.55, -.42, -.32, -.30, -.30, -.32, -.42, -.55,
    -.40, -.20,  .00,  .04,  .04,  .00, -.20, -.40,
    -.30,  .04,  .14,  .20,  .20,  .14,  .04, -.30,
    -.28,  .08,  .22,  .30,  .30,  .22,  .08, -.28,
    -.28,  .06,  .20,  .28,  .28,  .20,  .06, -.28,
    -.30,  .04,  .14,  .18,  .18,  .14,  .04, -.30,
    -.40, -.22,  .00,  .04,  .04,  .00, -.22, -.40,
    -.55, -.42, -.32, -.30, -.30, -.32, -.42, -.55
};

constexpr float KNIGHT_EG[64] = {
    -.60, -.42, -.32, -.28, -.28, -.32, -.42, -.60,
    -.42, -.22, -.08,  .02,  .02, -.08, -.22, -.42,
    -.32, -.06,  .10,  .16,  .16,  .10, -.06, -.32,
    -.28,  .02,  .16,  .24,  .24,  .16,  .02, -.28,
    -.28,  .02,  .16,  .24,  .24,  .16,  .02, -.28,
    -.32, -.06,  .10,  .16,  .16,  .10, -.06, -.32,
    -.42, -.22, -.08,  .02,  .02, -.08, -.22, -.42,
    -.60, -.42, -.32, -.28, -.28, -.32, -.42, -.60
};

constexpr float BISHOP_MG[64] = {
    -.22, -.12, -.10, -.10, -.10, -.10, -.12, -.22,
    -.12,  .06,  .04,  .06,  .06,  .04,  .06, -.12,
    -.10,  .06,  .12,  .16,  .16,  .12,  .06, -.10,
    -.10,  .10,  .14,  .20,  .20,  .14,  .10, -.10,
    -.10,  .06,  .16,  .18,  .18,  .16,  .06, -.10,
    -.10,  .14,  .14,  .14,  .14,  .14,  .14, -.10,
    -.12,  .10,  .06,  .04,  .04,  .06,  .10, -.12,
    -.22, -.12, -.10, -.10, -.10, -.10, -.12, -.22
};

constexpr float BISHOP_EG[64] = {
    -.16, -.08, -.06, -.06, -.06, -.06, -.08, -.16,
    -.08,  .04,  .06,  .06,  .06,  .06,  .04, -.08,
    -.06,  .08,  .12,  .14,  .14,  .12,  .08, -.06,
    -.06,  .08,  .14,  .18,  .18,  .14,  .08, -.06,
    -.06,  .08,  .14,  .18,  .18,  .14,  .08, -.06,
    -.06,  .08,  .12,  .14,  .14,  .12,  .08, -.06,
    -.08,  .04,  .06,  .06,  .06,  .06,  .04, -.08,
    -.16, -.08, -.06, -.06, -.06, -.06, -.08, -.16
};

constexpr float ROOK_MG[64] = {
     .04,  .04,  .04,  .08,  .08,  .04,  .04,  .04,
    -.04,  .00,  .00,  .00,  .00,  .00,  .00, -.04,
    -.04,  .00,  .00,  .00,  .00,  .00,  .00, -.04,
    -.04,  .00,  .00,  .00,  .00,  .00,  .00, -.04,
    -.04,  .00,  .00,  .00,  .00,  .00,  .00, -.04,
    -.04,  .00,  .00,  .00,  .00,  .00,  .00, -.04,
     .10,  .14,  .14,  .14,  .14,  .14,  .14,  .10,
     .02,  .02,  .04,  .08,  .08,  .04,  .02,  .02
};

constexpr float ROOK_EG[64] = {
     .06,  .06,  .06,  .08,  .08,  .06,  .06,  .06,
     .10,  .14,  .14,  .14,  .14,  .14,  .14,  .10,
     .04,  .06,  .06,  .06,  .06,  .06,  .06,  .04,
     .02,  .04,  .04,  .04,  .04,  .04,  .04,  .02,
     .00,  .02,  .02,  .02,  .02,  .02,  .02,  .00,
     .00,  .00,  .00,  .00,  .00,  .00,  .00,  .00,
     .00,  .00,  .00,  .00,  .00,  .00,  .00,  .00,
     .02,  .02,  .04,  .06,  .06,  .04,  .02,  .02
};

constexpr float QUEEN_MG[64] = {
    -.18, -.10, -.08, -.04, -.04, -.08, -.10, -.18,
    -.10,  .00,  .02,  .02,  .02,  .02,  .00, -.10,
    -.08,  .02,  .05,  .06,  .06,  .05,  .02, -.08,
    -.04,  .02,  .06,  .08,  .08,  .06,  .02, -.04,
    -.04,  .02,  .06,  .08,  .08,  .06,  .02, -.04,
    -.08,  .04,  .06,  .06,  .06,  .06,  .02, -.08,
    -.10,  .00,  .04,  .02,  .02,  .00,  .00, -.10,
    -.18, -.10, -.08, -.04, -.04, -.08, -.10, -.18
};

constexpr float QUEEN_EG[64] = {
    -.10, -.06, -.04, -.02, -.02, -.04, -.06, -.10,
    -.06,  .00,  .02,  .02,  .02,  .02,  .00, -.06,
    -.04,  .02,  .05,  .06,  .06,  .05,  .02, -.04,
    -.02,  .02,  .06,  .08,  .08,  .06,  .02, -.02,
    -.02,  .02,  .06,  .08,  .08,  .06,  .02, -.02,
    -.04,  .02,  .05,  .06,  .06,  .05,  .02, -.04,
    -.06,  .00,  .02,  .02,  .02,  .02,  .00, -.06,
    -.10, -.06, -.04, -.02, -.02, -.04, -.06, -.10
};

constexpr float KING_MG[64] = {
    -.35, -.45, -.45, -.55, -.55, -.45, -.45, -.35,
    -.35, -.45, -.45, -.55, -.55, -.45, -.45, -.35,
    -.35, -.45, -.45, -.55, -.55, -.45, -.45, -.35,
    -.35, -.45, -.45, -.55, -.55, -.45, -.45, -.35,
    -.25, -.35, -.35, -.45, -.45, -.35, -.35, -.25,
    -.10, -.20, -.20, -.25, -.25, -.20, -.20, -.10,
     .25,  .25,  .05,  .00,  .00,  .05,  .25,  .25,
     .30,  .40,  .15,  .00,  .00,  .15,  .40,  .30
};

constexpr float KING_EG[64] = {
    -.45, -.25, -.15, -.10, -.10, -.15, -.25, -.45,
    -.25, -.05,  .05,  .10,  .10,  .05, -.05, -.25,
    -.15,  .05,  .18,  .24,  .24,  .18,  .05, -.15,
    -.10,  .10,  .24,  .32,  .32,  .24,  .10, -.10,
    -.10,  .10,  .24,  .32,  .32,  .24,  .10, -.10,
    -.15,  .05,  .18,  .24,  .24,  .18,  .05, -.15,
    -.25, -.05,  .05,  .10,  .10,  .05, -.05, -.25,
    -.45, -.25, -.15, -.10, -.10, -.15, -.25, -.45
};

// -----------------------------------------------------------------------
// Baked tables: base piece value + PST bonus, indexed by [pieceType][sq].
// pieceType: 1=pawn, 2=knight, 3=bishop, 4=rook, 5=queen, 6=king
// -----------------------------------------------------------------------
static constexpr auto makeBakedTables() {
    struct Tables {
        float mg[7][64]{};
        float eg[7][64]{};
    } t;

    constexpr float valMg[7] = { 0, 1.00f, 3.20f, 3.33f, 5.10f, 9.50f, 0 };
    constexpr float valEg[7] = { 0, 1.15f, 3.00f, 3.30f, 5.20f, 9.30f, 0 };

    constexpr const float* MGT[7] = { nullptr, PAWN_MG, KNIGHT_MG, BISHOP_MG, ROOK_MG, QUEEN_MG, KING_MG };
    constexpr const float* EGT[7] = { nullptr, PAWN_EG, KNIGHT_EG, BISHOP_EG, ROOK_EG, QUEEN_EG, KING_EG };

    for (int pt = 1; pt <= 6; pt++) {
        for (int sq = 0; sq < 64; sq++) {
            t.mg[pt][sq] = valMg[pt] + MGT[pt][sq];
            t.eg[pt][sq] = valEg[pt] + EGT[pt][sq];
        }
    }
    return t;
}
static constexpr auto BAKED = makeBakedTables();

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------

inline bool isBlackPiece(Piece p) { return p != EMPTY && (p >> 3); }
inline bool isWhitePiece(Piece p) { return p != EMPTY && !(p >> 3); }

inline int fileOf(int idx) { return idx & 7; }
inline int rankOf(int idx) { return idx >> 3; }

inline int pieceType(Piece p) { return p & 7; }

inline float pieceValueMg(Piece p) {
    switch (pieceType(p)) {
        case 1: return 1.00f;
        case 2: return 3.20f;
        case 3: return 3.33f;
        case 4: return 5.10f;
        case 5: return 9.50f;
        default: return 0.0f;
    }
}
inline float pieceValueEg(Piece p) {
    switch (pieceType(p)) {
        case 1: return 1.15f;
        case 2: return 3.00f;
        case 3: return 3.30f;
        case 4: return 5.20f;
        case 5: return 9.30f;
        default: return 0.0f;
    }
}

inline int manhattan(Position a, Position b) {
    return std::abs(a.rank() - b.rank()) + std::abs(a.file() - b.file());
}

inline float tableMg(Piece p, int idx) {
    if (isBlackPiece(p)) idx ^= 56;
    return BAKED.mg[p & 7][idx];
}
inline float tableEg(Piece p, int idx) {
    if (isBlackPiece(p)) idx ^= 56;
    return BAKED.eg[p & 7][idx];
}

inline int gamePhaseWeight(Piece p) {
    switch (pieceType(p)) {
        case 2: case 3: return 1;
        case 4:         return 2;
        case 5:         return 4;
        default:        return 0;
    }
}

inline bool sameColor(Piece p, bool black) {
    return p != EMPTY && bool(p >> 3) == black;
}
inline bool enemyColor(Piece p, bool black) {
    return p != EMPTY && bool(p >> 3) != black;
}

inline Piece evalPieceAt(const Board& board, int rank, int file) {
    if (rank < 0 || rank >= 8 || file < 0 || file >= 8) return EMPTY;
    return Piece((board.getRankBits(rank) >> ((7 - file) * 4)) & 0xF);
}

// Fast bitboard-based piece lookup (prefer over evalPieceAt in hot loops)
inline Piece evalPieceAtBB(const Board& board, int sq) {
    for (int pt = 1; pt <= 6; pt++) {
        if (board.getPieceBits(static_cast<Piece>(pt)) & (1ULL << sq))
            return static_cast<Piece>(pt);
        if (board.getPieceBits(static_cast<Piece>(pt | 8)) & (1ULL << sq))
            return static_cast<Piece>(pt | 8);
    }
    return EMPTY;
}

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

inline int popLsbIndex(uint64_t& bits) {
    int index = __builtin_ctzll(bits);
    bits &= bits - 1;
    return index;
}

inline void collectPawnData(uint64_t bits, EvalSideData& side) {
    while (bits != 0) {
        int idx = popLsbIndex(bits);
        Position p(idx);
        int r = p.rank();
        int f = p.file();

        side.pawnFiles[f]++;
        side.pawnFileMask |= (1u << f);
        if (side.pawnCount < 8) side.pawns[side.pawnCount++] = p;

        if (r < side.pawnMinRank[f]) side.pawnMinRank[f] = r;
        if (r > side.pawnMaxRank[f]) side.pawnMaxRank[f] = r;
    }
}

inline void collectPiecePositions(uint64_t bits, Position positions[], int& count, int maxCount) {
    while (bits != 0) {
        int idx = popLsbIndex(bits);
        if (count < maxCount) positions[count++] = Position(idx);
    }
}

inline void collectKingPosition(uint64_t bits, EvalSideData& side) {
    if (bits == 0) return;
    side.kingPos = Position(popLsbIndex(bits));
    side.hasKing = true;
}

inline bool squareInMask(int rank, int file, uint64_t mask) {
    if (rank < 0 || rank >= 8 || file < 0 || file >= 8) return false;
    return (mask & (1ULL << (rank * 8 + file))) != 0;
}

// -----------------------------------------------------------------------
// King zone mask: 3x3 around king + extended row in front
// -----------------------------------------------------------------------
inline uint64_t kingZoneMask(Position king, bool blackKing) {
    if (king.isNone()) return 0;

    uint64_t mask = 0;
    int kr = king.rank();
    int kf = king.file();

    for (int dr = -1; dr <= 1; dr++) {
        for (int df = -1; df <= 1; df++) {
            int r = kr + dr, f = kf + df;
            if (r >= 0 && r < 8 && f >= 0 && f < 8)
                mask |= 1ULL << (r * 8 + f);
        }
    }

    int front = blackKing ? -1 : 1;
    for (int df = -2; df <= 2; df++) {
        int r = kr + front, f = kf + df;
        if (r >= 0 && r < 8 && f >= 0 && f < 8)
            mask |= 1ULL << (r * 8 + f);
    }

    return mask;
}

// -----------------------------------------------------------------------
// Mobility
// -----------------------------------------------------------------------
inline int knightMobility(Position from, uint64_t ownPieces) {
    int r = from.rank(), f = from.file(), count = 0;
    for (const auto& o : knightOffsets) {
        int nr = r + o[0], nf = f + o[1];
        if (nr < 0 || nr >= 8 || nf < 0 || nf >= 8) continue;
        if (!(ownPieces >> (nr * 8 + nf) & 1)) count++;
    }
    return count;
}

inline int slidingMobility(Position from, uint64_t occ, uint64_t ownPieces,
                            const int dirs[][2], int numDirs)
{
    int r = from.rank(), f = from.file(), count = 0;
    for (int d = 0; d < numDirs; d++) {
        int nr = r, nf = f;
        for (int step = 1; step < 8; step++) {
            nr += dirs[d][0]; nf += dirs[d][1];
            if (nr < 0 || nr >= 8 || nf < 0 || nf >= 8) break;
            int sq = nr * 8 + nf;
            if (ownPieces >> sq & 1) break;
            count++;
            if (occ >> sq & 1) break;
        }
    }
    return count;
}

inline float evalMobilityCached(const Board& board, const EvalSideData& white, const EvalSideData& black,
                                 float midgameT, float endgameT)
{
    uint64_t whiteOcc = 0, blackOcc = 0;
    for (int p = 1; p <= 6; p++)  whiteOcc |= board.getPieceBits(static_cast<Piece>(p));
    for (int p = 9; p <= 14; p++) blackOcc |= board.getPieceBits(static_cast<Piece>(p));
    uint64_t allOcc = whiteOcc | blackOcc;

    constexpr float KNIGHT_MOB_MG = 0.042f, KNIGHT_MOB_EG = 0.040f;
    constexpr float BISHOP_MOB_MG = 0.040f, BISHOP_MOB_EG = 0.042f;
    constexpr float ROOK_MOB_MG   = 0.024f, ROOK_MOB_EG   = 0.040f;
    constexpr float QUEEN_MOB_MG  = 0.010f, QUEEN_MOB_EG  = 0.020f;

    float wScore = 0.0f, bScore = 0.0f;

    for (int i = 0; i < white.knightCount; i++)
        wScore += knightMobility(white.knights[i], whiteOcc) * (KNIGHT_MOB_MG * midgameT + KNIGHT_MOB_EG * endgameT);
    for (int i = 0; i < black.knightCount; i++)
        bScore += knightMobility(black.knights[i], blackOcc) * (KNIGHT_MOB_MG * midgameT + KNIGHT_MOB_EG * endgameT);

    for (int i = 0; i < white.bishops; i++)
        wScore += slidingMobility(white.bishopSquares[i], allOcc, whiteOcc, bishopDirections, 4)
                  * (BISHOP_MOB_MG * midgameT + BISHOP_MOB_EG * endgameT);
    for (int i = 0; i < black.bishops; i++)
        bScore += slidingMobility(black.bishopSquares[i], allOcc, blackOcc, bishopDirections, 4)
                  * (BISHOP_MOB_MG * midgameT + BISHOP_MOB_EG * endgameT);

    for (int i = 0; i < white.rookCount; i++)
        wScore += slidingMobility(white.rooks[i], allOcc, whiteOcc, rookDirections, 4)
                  * (ROOK_MOB_MG * midgameT + ROOK_MOB_EG * endgameT);
    for (int i = 0; i < black.rookCount; i++)
        bScore += slidingMobility(black.rooks[i], allOcc, blackOcc, rookDirections, 4)
                  * (ROOK_MOB_MG * midgameT + ROOK_MOB_EG * endgameT);

    for (int i = 0; i < white.queenCount; i++)
        wScore += slidingMobility(white.queens[i], allOcc, whiteOcc, queenDirections, 8)
                  * (QUEEN_MOB_MG * midgameT + QUEEN_MOB_EG * endgameT);
    for (int i = 0; i < black.queenCount; i++)
        bScore += slidingMobility(black.queens[i], allOcc, blackOcc, queenDirections, 8)
                  * (QUEEN_MOB_MG * midgameT + QUEEN_MOB_EG * endgameT);

    return wScore - bScore;
}

// -----------------------------------------------------------------------
// King attack evaluation
// FIX: countSlidingKingZoneAttacks now correctly stops the ray at the
// first blocking piece — previously it counted zone squares even when
// the ray was physically blocked by an intervening piece.
// -----------------------------------------------------------------------
inline int countPawnKingZoneAttacks(Position from, bool black, uint64_t zone) {
    int dir = black ? -1 : 1;
    int r = from.rank(), f = from.file(), attacks = 0;
    attacks += squareInMask(r + dir, f - 1, zone) ? 1 : 0;
    attacks += squareInMask(r + dir, f + 1, zone) ? 1 : 0;
    return attacks;
}

template <size_t N>
inline int countSlidingKingZoneAttacks(
    const Board& board, Position from, bool attackerBlack,
    const int (&directions)[N][2], uint64_t zone)
{
    int attacks = 0, sr = from.rank(), sf = from.file();
    for (const auto& direction : directions) {
        int rank = sr, file = sf;
        for (int d = 1; d < 8; d++) {
            rank += direction[0]; file += direction[1];
            if (rank < 0 || rank >= 8 || file < 0 || file >= 8) break;
            int idx = rank * 8 + file;
            bool inZone = (zone & (1ULL << idx)) != 0;
            Piece piece = evalPieceAt(board, rank, file);

            if (piece != EMPTY) {
                // The blocking piece itself may be inside the zone — count it,
                // then stop: nothing behind a blocker can attack through it.
                if (inZone) attacks++;
                break;
            }
            if (inZone) attacks++;
        }
    }
    return attacks;
}

inline float evalKingAttackCached(
    const Board& board,
    const EvalSideData& attacker,
    const EvalSideData& defender,
    bool attackerBlack)
{
    if (!defender.hasKing) return 0.0f;

    bool defenderBlack = !attackerBlack;
    uint64_t zone = kingZoneMask(defender.kingPos, defenderBlack);
    if (zone == 0) return 0.0f;

    float score = 0.0f;
    int attackUnits = 0;
    int attackingPieces = 0;

    auto addAttacks = [&](int attacks, int units, float value) {
        if (attacks <= 0) return;
        score += value * float(attacks);
        attackUnits += attacks * units;
        attackingPieces++;
    };

    // Pawns
    for (int i = 0; i < attacker.pawnCount; i++) {
        int attacks = countPawnKingZoneAttacks(attacker.pawns[i], attackerBlack, zone);
        score += 0.06f * float(attacks);
        attackUnits += attacks;
    }

    // Knights
    for (int i = 0; i < attacker.knightCount; i++) {
        int r = attacker.knights[i].rank(), f = attacker.knights[i].file();
        int attacks = 0;
        for (const auto& o : knightOffsets) {
            int nr = r + o[0], nf = f + o[1];
            if (squareInMask(nr, nf, zone)) attacks++;
        }
        addAttacks(attacks, 2, 0.08f);
    }

    // Bishops
    for (int i = 0; i < attacker.bishops; i++) {
        int raw = countSlidingKingZoneAttacks(board, attacker.bishopSquares[i], attackerBlack, bishopDirections, zone);
        addAttacks(raw, 2, 0.07f);
    }

    // Rooks
    for (int i = 0; i < attacker.rookCount; i++) {
        int raw = countSlidingKingZoneAttacks(board, attacker.rooks[i], attackerBlack, rookDirections, zone);
        addAttacks(raw, 3, 0.12f);
    }

    // Queens
    for (int i = 0; i < attacker.queenCount; i++) {
        int raw = countSlidingKingZoneAttacks(board, attacker.queens[i], attackerBlack, queenDirections, zone);
        addAttacks(raw, 4, 0.08f);
    }

    // Coordination bonus
    if (attackingPieces >= 2)
        score += 0.015f * float(attackingPieces - 1) * float(attackUnits);

    // Queen amplifier
    if (attacker.queenCount > 0 && attackingPieces >= 2)
        score += 0.06f;

    // Gate: minor-only attacks without heavy backup are rarely decisive
    bool hasHeavyAttacker = attacker.queenCount > 0 || attacker.rookCount >= 2;
    if (!hasHeavyAttacker && attackingPieces < 3)
        score *= 0.28f;

    // Pawn shelter / open file penalties
    bool attackerHasSlider = attacker.rookCount > 0 || attacker.queenCount > 0;
    int kingFile = defender.kingPos.file();
    int kingRank = defender.kingPos.rank();

    for (int df = -1; df <= 1; df++) {
        int file = kingFile + df;
        if (file < 0 || file >= 8) continue;

        bool defenderPawn  = defender.pawnFiles[file] > 0;
        bool attackerPawn  = attacker.pawnFiles[file] > 0;
        bool shelterPawn   = false;

        if (defenderPawn) {
            if (defenderBlack) {
                int closest = defender.pawnMaxRank[file];
                shelterPawn = closest < kingRank && closest >= kingRank - 3;
            } else {
                int closest = defender.pawnMinRank[file];
                shelterPawn = closest > kingRank && closest <= kingRank + 3;
            }
        }

        if (!defenderPawn && !attackerPawn)
            score += attackerHasSlider ? 0.10f : 0.04f;
        else if (!defenderPawn)
            score += attackerHasSlider ? 0.06f : 0.02f;

        if (!shelterPawn)
            score += (df == 0) ? 0.09f : 0.05f;
    }

    return score;
}

// -----------------------------------------------------------------------
// Pawn structure
// -----------------------------------------------------------------------
inline float evalPawnStructureCached(const EvalSideData& side, const EvalSideData& enemy,
                                      bool black, float endgameT)
{
    float score = 0.0f;

    for (int i = 0; i < side.pawnCount; i++) {
        Position p = side.pawns[i];
        int r = p.rank(), f = p.file();
        int advanceRank = black ? 7 - r : r;

        // ---- Doubled pawns ----
        if (side.pawnFiles[f] > 1)
            score -= 0.10f * float(side.pawnFiles[f] - 1);

        // ---- Isolated pawns ----
        bool isolated = !(side.pawnFileMask & (
            (f > 0 ? (1u << (f - 1)) : 0u) |
            (f < 7 ? (1u << (f + 1)) : 0u)));
        if (isolated)
            score -= 0.14f;

        // ---- Backward pawn ----
        if (!isolated) {
            bool hasSupportBehind = false;
            for (int df = -1; df <= 1; df += 2) {
                int ff = f + df;
                if (ff < 0 || ff >= 8 || side.pawnFiles[ff] == 0) continue;
                if (!black && side.pawnMinRank[ff] < r) { hasSupportBehind = true; break; }
                if ( black && side.pawnMaxRank[ff] > r) { hasSupportBehind = true; break; }
            }
            if (!hasSupportBehind) {
                int stopRank = black ? r - 1 : r + 1;
                bool stopAttacked = false;
                for (int df = -1; df <= 1; df += 2) {
                    int ff = f + df;
                    if (ff < 0 || ff >= 8 || enemy.pawnFiles[ff] == 0) continue;
                    int attackRank = black ? stopRank + 1 : stopRank - 1;
                    if (enemy.pawnMinRank[ff] <= attackRank && enemy.pawnMaxRank[ff] >= attackRank) {
                        stopAttacked = true; break;
                    }
                }
                if (stopAttacked) score -= 0.10f;
            }
        }

        // ---- Passed pawn ----
        bool passed = true;
        for (int df = -1; df <= 1 && passed; df++) {
            int ff = f + df;
            if (ff < 0 || ff >= 8 || enemy.pawnFiles[ff] == 0) continue;
            if (!black) {
                if (enemy.pawnMaxRank[ff] > r) passed = false;
            } else {
                if (enemy.pawnMinRank[ff] < r) passed = false;
            }
        }

        if (passed) {
            static constexpr float passedBonus[8] = {
                0.0f, 0.06f, 0.14f, 0.28f,
                0.52f, 0.90f, 1.60f, 0.0f
            };
            float bonus = passedBonus[advanceRank];
            bonus *= 1.0f + 1.8f * endgameT;
            score += bonus;

            if (advanceRank >= 5)
                score += 0.14f * float(advanceRank - 4);
        }
    }

    return score;
}

// -----------------------------------------------------------------------
// Rooks: open/half-open files, seventh rank
// -----------------------------------------------------------------------
inline float evalRooksCached(const EvalSideData& side, const EvalSideData& enemy, bool black) {
    float score = 0.0f;
    int seventh = black ? 1 : 6;

    for (int i = 0; i < side.rookCount; i++) {
        int f = side.rooks[i].file();
        bool ownPawn   = side.pawnFiles[f] > 0;
        bool enemyPawn = enemy.pawnFiles[f] > 0;

        if (!ownPawn && !enemyPawn) score += 0.28f;
        else if (!ownPawn)          score += 0.14f;

        if (side.rooks[i].rank() == seventh) score += 0.20f;
    }

    // Battery bonus
    for (int i = 0; i < side.rookCount; i++) {
        for (int j = i + 1; j < side.rookCount; j++) {
            if (side.rooks[i].file() == side.rooks[j].file() ||
                side.rooks[i].rank() == side.rooks[j].rank())
                score += 0.18f;
        }
    }

    return score;
}

inline float rookBehindPassedPawn(const EvalSideData& side, const EvalSideData& enemy, bool black) {
    float score = 0.0f;
    for (int i = 0; i < side.rookCount; i++) {
        int rf = side.rooks[i].file(), rr = side.rooks[i].rank();
        for (int j = 0; j < side.pawnCount; j++) {
            if (side.pawns[j].file() != rf) continue;
            int pr = side.pawns[j].rank();

            bool passed = true;
            for (int df = -1; df <= 1 && passed; df++) {
                int ff = rf + df;
                if (ff < 0 || ff >= 8 || enemy.pawnFiles[ff] == 0) continue;
                if (!black && enemy.pawnMaxRank[ff] > pr) passed = false;
                if ( black && enemy.pawnMinRank[ff] < pr) passed = false;
            }
            if (!passed) continue;

            bool rookBehind = black ? rr > pr : rr < pr;
            if (rookBehind) score += 0.30f;
        }
    }
    return score;
}

// -----------------------------------------------------------------------
// King shield
// -----------------------------------------------------------------------
inline float evalKingShieldCached(const Board& board, const EvalSideData& side, bool black) {
    if (!side.hasKing) return 0.0f;

    Piece pawn = black ? BLACK_PAWN : WHITE_PAWN;
    float score = 0.0f;
    int dir = black ? -1 : 1;
    int kr = side.kingPos.rank(), kf = side.kingPos.file();

    for (int df = -1; df <= 1; df++) {
        int r = kr + dir, f = kf + df;
        if (r < 0 || r >= 8 || f < 0 || f >= 8) continue;
        if (evalPieceAt(board, r, f) == pawn) score += 0.10f;
        else                                  score -= 0.08f;
    }

    return score;
}

// -----------------------------------------------------------------------
// Center control
// -----------------------------------------------------------------------
inline float evalCenterCached(const Board& board, bool black) {
    float score = 0.0f;
    constexpr int centers[4] = { 3*8+3, 3*8+4, 4*8+3, 4*8+4 };
    for (int idx : centers) {
        if (sameColor(evalPieceAt(board, idx >> 3, idx & 7), black))
            score += 0.10f;
    }
    return score;
}

// -----------------------------------------------------------------------
// Space evaluation
// -----------------------------------------------------------------------
inline float evalSpace(const EvalSideData& side, bool black) {
    float score = 0.0f;
    for (int i = 0; i < side.pawnCount; i++) {
        int advRank = black ? 7 - side.pawns[i].rank() : side.pawns[i].rank();
        if (advRank >= 4) score += 0.06f * float(advRank - 3);
    }
    int oppHalfStart = black ? 0 : 4;
    int oppHalfEnd   = black ? 3 : 7;
    for (int i = 0; i < side.knightCount; i++) {
        int r = side.knights[i].rank();
        if (r >= oppHalfStart && r <= oppHalfEnd) score += 0.04f;
    }
    for (int i = 0; i < side.bishops; i++) {
        int r = side.bishopSquares[i].rank();
        if (r >= oppHalfStart && r <= oppHalfEnd) score += 0.04f;
    }
    return score;
}

// -----------------------------------------------------------------------
// Outpost detection
// -----------------------------------------------------------------------
inline bool isOutpostSquare(int rank, int file, const EvalSideData& enemy, bool pieceIsBlack) {
    for (int df = -1; df <= 1; df += 2) {
        int ff = file + df;
        if (ff < 0 || ff >= 8 || enemy.pawnFiles[ff] == 0) continue;
        if (!pieceIsBlack) {
            if (enemy.pawnMinRank[ff] < rank) return false;
        } else {
            if (enemy.pawnMaxRank[ff] > rank) return false;
        }
    }
    return true;
}

inline bool isSupportedByPawn(int rank, int file, const EvalSideData& side, bool pieceIsBlack) {
    int behindRank = pieceIsBlack ? rank + 1 : rank - 1;
    for (int df = -1; df <= 1; df += 2) {
        int ff = file + df;
        if (ff < 0 || ff >= 8 || side.pawnFiles[ff] == 0) continue;
        if (side.pawnMinRank[ff] <= behindRank && side.pawnMaxRank[ff] >= behindRank)
            return true;
    }
    return false;
}

inline float evalOutposts(const EvalSideData& side, const EvalSideData& enemy, bool black, float midgameT) {
    float score = 0.0f;
    for (int i = 0; i < side.knightCount; i++) {
        int r = side.knights[i].rank(), f = side.knights[i].file();
        if (isOutpostSquare(r, f, enemy, black)) {
            float bonus = 0.20f;
            if (isSupportedByPawn(r, f, side, black)) bonus += 0.14f;
            score += bonus * midgameT;
        }
    }
    for (int i = 0; i < side.bishops; i++) {
        int r = side.bishopSquares[i].rank(), f = side.bishopSquares[i].file();
        if (isOutpostSquare(r, f, enemy, black) && isSupportedByPawn(r, f, side, black)) {
            score += 0.10f * midgameT;
        }
    }
    return score;
}

// -----------------------------------------------------------------------
// Tempo / side-to-move bonus
// FIX: reduced from 0.10 to 0.03 — 10 centipawns was large enough to
// mask real material differences at shallow depth.
// -----------------------------------------------------------------------
constexpr float TEMPO_BONUS = 0.03f;

// -----------------------------------------------------------------------
// Insufficient material / draw heuristics
// FIX: removed the opposite-colored bishop approximation entirely.
// Without tracking square color we can't reliably detect opposite bishops,
// so the old code was scaling down positions with same-colored bishops too,
// causing the engine to give away material thinking it was a draw.
// -----------------------------------------------------------------------
inline float drawScaleFactor(const EvalSideData& white, const EvalSideData& black) {
    // KK, KN vs K, KB vs K — all theoretical draws
    if (white.rookCount == 0 && black.rookCount == 0 &&
        white.queenCount == 0 && black.queenCount == 0 &&
        white.pawnCount == 0 && black.pawnCount == 0)
    {
        int wMinors = white.knightCount + white.bishops;
        int bMinors = black.knightCount + black.bishops;

        if ((wMinors <= 1 && bMinors == 0) || (bMinors <= 1 && wMinors == 0))
            return 0.0f;

        // KNN vs K — generally a draw
        if ((white.knightCount == 2 && white.bishops == 0 && bMinors == 0) ||
            (black.knightCount == 2 && black.bishops == 0 && wMinors == 0))
            return 0.05f;
    }

    return 1.0f;
}

// -----------------------------------------------------------------------
// Endgame king activity
// -----------------------------------------------------------------------
inline float endgameKingActivity(Position king) {
    int centerDist = std::min({
        manhattan(king, Position(3, 3)),
        manhattan(king, Position(3, 4)),
        manhattan(king, Position(4, 3)),
        manhattan(king, Position(4, 4))
    });
    return -0.09f * float(centerDist);
}

inline float kingSupportsPawns(const EvalSideData& side, const EvalSideData& enemy, bool black) {
    float score = 0.0f;
    for (int i = 0; i < side.pawnCount; i++) {
        int promotionRank = black ? 0 : 7;
        Position promo(promotionRank, side.pawns[i].file());
        int ownDist   = manhattan(side.kingPos, side.pawns[i]);
        int enemyDist = manhattan(enemy.kingPos, promo);
        score += 0.05f * float(6 - std::min(6, ownDist));
        score += 0.06f * float(enemyDist);
    }
    return score;
}

// -----------------------------------------------------------------------
// Mop-up
// -----------------------------------------------------------------------
inline float mopUpEval(const EvalSideData& winning, const EvalSideData& losing, float advantage) {
    if (!winning.hasKing || !losing.hasKing) return 0.0f;

    int lf = losing.kingPos.file(), lr = losing.kingPos.rank();
    int cornerDist = std::min(lf, 7 - lf) + std::min(lr, 7 - lr);
    int kingDist   = manhattan(winning.kingPos, losing.kingPos);

    float cornerPressure = 0.07f * float(6 - cornerDist);
    float kingProximity  = 0.04f * float(14 - kingDist);

    return advantage * (cornerPressure + kingProximity);
}

// -----------------------------------------------------------------------
// Pawn-king race
// -----------------------------------------------------------------------
inline float pawnKingRaceBonus(const EvalSideData& side, const EvalSideData& enemy, bool black) {
    float score = 0.0f;
    for (int i = 0; i < side.pawnCount; i++) {
        int promotionRank = black ? 0 : 7;
        int stepsToPromo = black ? side.pawns[i].rank() : 7 - side.pawns[i].rank();
        if (stepsToPromo <= 0) continue;

        Position promoSquare(promotionRank, side.pawns[i].file());
        int enemyKingSteps = manhattan(enemy.kingPos, promoSquare);

        int gap = enemyKingSteps - stepsToPromo;
        if (gap >= 2)       score += 2.50f;
        else if (gap >= 0)  score += 0.80f;
        else if (gap >= -1) score += 0.20f;
    }
    return score;
}

// -----------------------------------------------------------------------
// Castling status
// FIX: reduced the castled-king bonus from 0.38 to 0.18 and raised the
// castling-rights bonus to 0.12/0.10 so the eval cliff when castling
// occurs is much smaller. The pawn-shield eval already rewards castled
// kings; the old bonus on top caused the engine to overvalue castling
// and misread positions where castling wasn't the critical factor.
// -----------------------------------------------------------------------
inline float evalCastlingStatus(const Board& board, const EvalSideData& side, bool black) {
    if (!side.hasKing) return 0.0f;

    Position kingSideCastle  = black ? Position{7, 6} : Position{0, 6};
    Position queenSideCastle = black ? Position{7, 2} : Position{0, 2};

    if (side.kingPos == kingSideCastle || side.kingPos == queenSideCastle)
        return 0.18f;

    float score = 0.0f;
    if (board.canCastleKingSide(black))  score += 0.12f;
    if (board.canCastleQueenSide(black)) score += 0.10f;
    return score;
}

// -----------------------------------------------------------------------
// Threats: attacks on undefended / higher-value pieces
// -----------------------------------------------------------------------
inline float evalThreats(const Board& board, const EvalSideData& us,
                          const EvalSideData& them, bool usBlack)
{
    float score = 0.0f;
    Piece enemyPawn = usBlack ? WHITE_PAWN : BLACK_PAWN;

    // Knight attacks non-pawn enemy piece
    for (int i = 0; i < us.knightCount; i++) {
        int r = us.knights[i].rank(), f = us.knights[i].file();
        for (const auto& o : knightOffsets) {
            int nr = r + o[0], nf = f + o[1];
            if (nr < 0 || nr >= 8 || nf < 0 || nf >= 8) continue;
            Piece target = evalPieceAt(board, nr, nf);
            if (!enemyColor(target, usBlack) || target == enemyPawn) continue;
            score += 0.06f;
        }
    }

    // Pawn attacks non-pawn enemy piece
    int dir = usBlack ? -1 : 1;
    for (int i = 0; i < us.pawnCount; i++) {
        int r = us.pawns[i].rank(), f = us.pawns[i].file();
        for (int df : {-1, 1}) {
            int nr = r + dir, nf = f + df;
            if (nr < 0 || nr >= 8 || nf < 0 || nf >= 8) continue;
            Piece target = evalPieceAt(board, nr, nf);
            if (!enemyColor(target, usBlack) || target == enemyPawn) continue;
            score += 0.10f;
        }
    }

    return score;
}

// -----------------------------------------------------------------------
// Piece safety (loose / undefended piece penalty)
// FIX: the old version penalised any attacked piece, even well-defended
// ones, causing the engine to flee pieces unnecessarily and badly
// miscount material balance. New version only penalises pieces that are
// attacked AND undefended — a piece with a defender behind it is fine.
// -----------------------------------------------------------------------
inline float loosePiecePenalty(const Board& board, Position position, Piece piece,
                                bool black, float endgameT)
{
    if (piece == EMPTY || pieceType(piece) == 6) return 0.0f;

    // Not attacked at all — nothing to worry about
    if (!board.isSquareAttacked(position, !black)) return 0.0f;

    // Attacked but defended — the attacker takes a risk too; don't penalise
    if (board.isSquareAttacked(position, black)) return 0.0f;

    // Attacked and completely undefended: flat 25% of piece value as penalty
    float value = pieceValueMg(piece) * (1.0f - endgameT) + pieceValueEg(piece) * endgameT;
    return value * 0.25f;
}

inline float evalPieceSafetyCached(const Board& board, const EvalSideData& side,
                                    bool black, float endgameT)
{
    float penalty = 0.0f;
    Piece pawn   = black ? BLACK_PAWN   : WHITE_PAWN;
    Piece knight = black ? BLACK_KNIGHT : WHITE_KNIGHT;
    Piece bishop = black ? BLACK_BISHOP : WHITE_BISHOP;
    Piece rook   = black ? BLACK_ROOK   : WHITE_ROOK;
    Piece queen  = black ? BLACK_QUEEN  : WHITE_QUEEN;

    for (int i = 0; i < side.pawnCount;   i++) penalty += loosePiecePenalty(board, side.pawns[i],         pawn,   black, endgameT);
    for (int i = 0; i < side.knightCount; i++) penalty += loosePiecePenalty(board, side.knights[i],       knight, black, endgameT);
    for (int i = 0; i < side.bishops;     i++) penalty += loosePiecePenalty(board, side.bishopSquares[i], bishop, black, endgameT);
    for (int i = 0; i < side.rookCount;   i++) penalty += loosePiecePenalty(board, side.rooks[i],         rook,   black, endgameT);
    for (int i = 0; i < side.queenCount;  i++) penalty += loosePiecePenalty(board, side.queens[i],        queen,  black, endgameT);

    return -penalty;
}

// -----------------------------------------------------------------------
// Main evaluation
// -----------------------------------------------------------------------
[[nodiscard]] inline float evaluateBoard(const Board& board) {
    EvalSideData white, black;

    collectPawnData(board.getPieceBits(WHITE_PAWN), white);
    collectPawnData(board.getPieceBits(BLACK_PAWN), black);

    collectPiecePositions(board.getPieceBits(WHITE_ROOK),   white.rooks,         white.rookCount,   12);
    collectPiecePositions(board.getPieceBits(BLACK_ROOK),   black.rooks,         black.rookCount,   12);
    collectPiecePositions(board.getPieceBits(WHITE_KNIGHT), white.knights,       white.knightCount, 12);
    collectPiecePositions(board.getPieceBits(BLACK_KNIGHT), black.knights,       black.knightCount, 12);
    collectPiecePositions(board.getPieceBits(WHITE_BISHOP), white.bishopSquares, white.bishops,     12);
    collectPiecePositions(board.getPieceBits(BLACK_BISHOP), black.bishopSquares, black.bishops,     12);
    collectPiecePositions(board.getPieceBits(WHITE_QUEEN),  white.queens,        white.queenCount,  12);
    collectPiecePositions(board.getPieceBits(BLACK_QUEEN),  black.queens,        black.queenCount,  12);

    collectKingPosition(board.getPieceBits(WHITE_KING), white);
    collectKingPosition(board.getPieceBits(BLACK_KING), black);

    // Phase interpolation
    int phase = board.getEvalPhase();
    float endgameT  = 1.0f - std::min(24.0f, float(phase)) / 24.0f;
    float midgameT  = 1.0f - endgameT;
    float pureEndgameT = 1.0f - std::min(8.0f, float(phase)) / 8.0f;

    // --- Tapered PST scores ---
    float whiteScore = board.getWhiteMg() * midgameT + board.getWhiteEg() * endgameT;
    float blackScore = board.getBlackMg() * midgameT + board.getBlackEg() * endgameT;

    // --- Pawn structure ---
    whiteScore += evalPawnStructureCached(white, black, false, endgameT);
    blackScore += evalPawnStructureCached(black, white, true,  endgameT);

    // --- Rooks ---
    whiteScore += evalRooksCached(white, black, false);
    blackScore += evalRooksCached(black, white, true);
    whiteScore += rookBehindPassedPawn(white, black, false) * endgameT;
    blackScore += rookBehindPassedPawn(black, white, true)  * endgameT;

    // --- Bishop pair ---
    if (white.bishops >= 2) whiteScore += 0.32f;
    if (black.bishops >= 2) blackScore += 0.32f;

    // --- Center control ---
    whiteScore += evalCenterCached(board, false) * midgameT;
    blackScore += evalCenterCached(board, true)  * midgameT;

    // --- Space ---
    whiteScore += evalSpace(white, false) * midgameT;
    blackScore += evalSpace(black, true)  * midgameT;

    // --- Outposts ---
    whiteScore += evalOutposts(white, black, false, midgameT);
    blackScore += evalOutposts(black, white, true,  midgameT);

    // --- King safety ---
    whiteScore += evalKingShieldCached(board, white, false) * midgameT;
    blackScore += evalKingShieldCached(board, black, true)  * midgameT;

    whiteScore += evalCastlingStatus(board, white, false) * midgameT;
    blackScore += evalCastlingStatus(board, black, true)  * midgameT;

    whiteScore += evalKingAttackCached(board, white, black, false) * midgameT;
    blackScore += evalKingAttackCached(board, black, white, true)  * midgameT;

    // --- Endgame king activity ---
    whiteScore += endgameKingActivity(white.kingPos) * endgameT;
    blackScore += endgameKingActivity(black.kingPos) * endgameT;

    whiteScore += kingSupportsPawns(white, black, false) * pureEndgameT;
    blackScore += kingSupportsPawns(black, white, true)  * pureEndgameT;

    // --- Mobility ---
    float mobilityDiff = evalMobilityCached(board, white, black, midgameT, endgameT);
    whiteScore += mobilityDiff > 0.0f ?  mobilityDiff : 0.0f;
    blackScore += mobilityDiff < 0.0f ? -mobilityDiff : 0.0f;

    // --- Threats ---
    whiteScore += evalThreats(board, white, black, false);
    blackScore += evalThreats(board, black, white, true);

    // --- Piece safety ---
    {
        float safetyWeight = 0.25f + 0.40f * endgameT;
        whiteScore += evalPieceSafetyCached(board, white, false, endgameT) * safetyWeight;
        blackScore += evalPieceSafetyCached(board, black, true,  endgameT) * safetyWeight;
    }

    // --- Mop-up in winning endgames ---
    float materialDiff = whiteScore - blackScore;
    if (std::abs(materialDiff) > 1.2f) {
        bool whiteWinning = materialDiff > 0;
        float mopUp = mopUpEval(
            whiteWinning ? white : black,
            whiteWinning ? black : white,
            std::min(std::abs(materialDiff), 5.0f)
        ) * pureEndgameT;
        if (whiteWinning) whiteScore += mopUp;
        else              blackScore += mopUp;
    }

    // --- Pawn-king race ---
    whiteScore += pawnKingRaceBonus(white, black, false) * endgameT;
    blackScore += pawnKingRaceBonus(black, white, true)  * endgameT;

    // --- Draw scale ---
    float scale = drawScaleFactor(white, black);
    float eval = (whiteScore - blackScore) * scale;

    // --- Tempo bonus for side to move ---
    eval += board.getPlayerTurn() ? -TEMPO_BONUS : TEMPO_BONUS;

    // Return from the perspective of the side to move
    return board.getPlayerTurn() ? -eval : eval;
}

#endif