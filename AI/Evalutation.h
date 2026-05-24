#ifndef EVALUTATION_H
#define EVALUTATION_H

#include <cmath>
#include <algorithm>

constexpr float INF_EVAL = 1000000.0f;

constexpr float PAWN_MG[64] = {
     0,0,0,0,0,0,0,0,
     .55,.55,.55,.35,.35,.55,.55,.55,
     .12,.12,.22,.32,.32,.22,.12,.12,
     .06,.06,.12,.28,.28,.12,.06,.06,
     .02,.02,.06,.22,.22,.06,.02,.02,
     .04,-.06,-.12,0,0,-.12,-.06,.04,
     .04,.10,.10,-.20,-.20,.10,.10,.04,
     0,0,0,0,0,0,0,0
};

constexpr float PAWN_EG[64] = {
     0,0,0,0,0,0,0,0,
     .95,.95,.95,.95,.95,.95,.95,.95,
     .55,.55,.60,.70,.70,.60,.55,.55,
     .28,.28,.36,.48,.48,.36,.28,.28,
     .12,.12,.18,.28,.28,.18,.12,.12,
     .04,.04,.06,.10,.10,.06,.04,.04,
     0,0,0,-.05,-.05,0,0,0,
     0,0,0,0,0,0,0,0
};

constexpr float KNIGHT_MG[64] = {
    -.55,-.42,-.32,-.30,-.30,-.32,-.42,-.55,
    -.40,-.20,0,.04,.04,0,-.20,-.40,
    -.30,.04,.12,.18,.18,.12,.04,-.30,
    -.28,.08,.20,.28,.28,.20,.08,-.28,
    -.28,.05,.18,.25,.25,.18,.05,-.28,
    -.30,.04,.12,.16,.16,.12,.04,-.30,
    -.40,-.22,0,.03,.03,0,-.22,-.40,
    -.55,-.42,-.32,-.30,-.30,-.32,-.42,-.55
};

constexpr float KNIGHT_EG[64] = {
    -.65,-.45,-.35,-.30,-.30,-.35,-.45,-.65,
    -.45,-.25,-.10,0,0,-.10,-.25,-.45,
    -.35,-.08,.08,.14,.14,.08,-.08,-.35,
    -.30,0,.14,.22,.22,.14,0,-.30,
    -.30,0,.14,.22,.22,.14,0,-.30,
    -.35,-.08,.08,.14,.14,.08,-.08,-.35,
    -.45,-.25,-.10,0,0,-.10,-.25,-.45,
    -.65,-.45,-.35,-.30,-.30,-.35,-.45,-.65
};

constexpr float BISHOP_MG[64] = {
    -.22,-.12,-.10,-.10,-.10,-.10,-.12,-.22,
    -.12,.04,.02,.04,.04,.02,.04,-.12,
    -.10,.04,.10,.14,.14,.10,.04,-.10,
    -.10,.08,.12,.18,.18,.12,.08,-.10,
    -.10,.05,.14,.16,.16,.14,.05,-.10,
    -.10,.12,.12,.12,.12,.12,.12,-.10,
    -.12,.08,.04,.02,.02,.04,.08,-.12,
    -.22,-.12,-.10,-.10,-.10,-.10,-.12,-.22
};

constexpr float BISHOP_EG[64] = {
    -.16,-.08,-.06,-.06,-.06,-.06,-.08,-.16,
    -.08,.04,.06,.06,.06,.06,.04,-.08,
    -.06,.08,.12,.14,.14,.12,.08,-.06,
    -.06,.08,.14,.18,.18,.14,.08,-.06,
    -.06,.08,.14,.18,.18,.14,.08,-.06,
    -.06,.08,.12,.14,.14,.12,.08,-.06,
    -.08,.04,.06,.06,.06,.06,.04,-.08,
    -.16,-.08,-.06,-.06,-.06,-.06,-.08,-.16
};

constexpr float ROOK_MG[64] = {
     .02,.02,.02,.06,.06,.02,.02,.02,
    -.04,0,0,0,0,0,0,-.04,
    -.04,0,0,0,0,0,0,-.04,
    -.04,0,0,0,0,0,0,-.04,
    -.04,0,0,0,0,0,0,-.04,
    -.04,0,0,0,0,0,0,-.04,
     .12,.16,.16,.16,.16,.16,.16,.12,
     .02,.02,.04,.08,.08,.04,.02,.02
};

constexpr float ROOK_EG[64] = {
     .05,.05,.05,.08,.08,.05,.05,.05,
     .10,.14,.14,.14,.14,.14,.14,.10,
     .04,.06,.06,.06,.06,.06,.06,.04,
     .02,.04,.04,.04,.04,.04,.04,.02,
     0,.02,.02,.02,.02,.02,.02,0,
     0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,
     .02,.02,.04,.06,.06,.04,.02,.02
};

constexpr float QUEEN_MG[64] = {
    -.18,-.10,-.08,-.04,-.04,-.08,-.10,-.18,
    -.10,0,.02,.02,.02,.02,0,-.10,
    -.08,.02,.05,.06,.06,.05,.02,-.08,
    -.04,.02,.06,.08,.08,.06,.02,-.04,
    -.02,.02,.06,.08,.08,.06,.02,-.04,
    -.08,.04,.06,.06,.06,.06,.02,-.08,
    -.10,0,.04,.02,.02,0,0,-.10,
    -.18,-.10,-.08,-.04,-.04,-.08,-.10,-.18
};

constexpr float QUEEN_EG[64] = {
    -.10,-.06,-.04,-.02,-.02,-.04,-.06,-.10,
    -.06,0,.02,.02,.02,.02,0,-.06,
    -.04,.02,.05,.06,.06,.05,.02,-.04,
    -.02,.02,.06,.08,.08,.06,.02,-.02,
    -.02,.02,.06,.08,.08,.06,.02,-.02,
    -.04,.02,.05,.06,.06,.05,.02,-.04,
    -.06,0,.02,.02,.02,.02,0,-.06,
    -.10,-.06,-.04,-.02,-.02,-.04,-.06,-.10
};

constexpr float KING_MG[64] = {
    -.35,-.45,-.45,-.55,-.55,-.45,-.45,-.35,
    -.35,-.45,-.45,-.55,-.55,-.45,-.45,-.35,
    -.35,-.45,-.45,-.55,-.55,-.45,-.45,-.35,
    -.35,-.45,-.45,-.55,-.55,-.45,-.45,-.35,
    -.25,-.35,-.35,-.45,-.45,-.35,-.35,-.25,
    -.10,-.20,-.20,-.25,-.25,-.20,-.20,-.10,
     .25,.25,.05,0,0,.05,.25,.25,
     .30,.40,.15,0,0,.15,.40,.30
};

constexpr float KING_EG[64] = {
    -.45,-.25,-.15,-.10,-.10,-.15,-.25,-.45,
    -.25,-.05,.05,.10,.10,.05,-.05,-.25,
    -.15,.05,.18,.24,.24,.18,.05,-.15,
    -.10,.10,.24,.32,.32,.24,.10,-.10,
    -.10,.10,.24,.32,.32,.24,.10,-.10,
    -.15,.05,.18,.24,.24,.18,.05,-.15,
    -.25,-.05,.05,.10,.10,.05,-.05,-.25,
    -.45,-.25,-.15,-.10,-.10,-.15,-.25,-.45
};

static constexpr auto makeBakedTables() {
    struct Tables {
        float mg[7][64]{};
        float eg[7][64]{};
    } t;

    constexpr float valMg[7] = { 0, 1.00f, 3.20f, 3.33f, 5.10f, 9.50f, 0 };
    constexpr float valEg[7] = { 0, 1.20f, 3.05f, 3.35f, 5.25f, 9.35f, 0 };

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

inline int mirrorIndex(int index) {
    return index ^ 56;
}

inline bool isBlackPiece(Piece p) {
    return p != EMPTY && (p >> 3);
}
inline bool isWhitePiece(Piece p) {
    return p != EMPTY && !(p >> 3);
}

inline int fileOf(int idx) { return idx & 7; }
inline int rankOf(int idx) { return idx >> 3; }

inline float pieceValueMg(Piece p) {
    switch (p) {
        case WHITE_PAWN: case BLACK_PAWN: return 1.00f;
        case WHITE_KNIGHT: case BLACK_KNIGHT: return 3.20f;
        case WHITE_BISHOP: case BLACK_BISHOP: return 3.33f;
        case WHITE_ROOK: case BLACK_ROOK: return 5.10f;
        case WHITE_QUEEN: case BLACK_QUEEN: return 9.50f;
        default: return 0.0f;
    }
}
inline float pieceValueEg(Piece p) {
    switch (p) {
        case WHITE_PAWN: case BLACK_PAWN: return 1.20f;
        case WHITE_KNIGHT: case BLACK_KNIGHT: return 3.05f;
        case WHITE_BISHOP: case BLACK_BISHOP: return 3.35f;
        case WHITE_ROOK: case BLACK_ROOK: return 5.25f;
        case WHITE_QUEEN: case BLACK_QUEEN: return 9.35f;
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
    switch (p) {
        case WHITE_KNIGHT: case BLACK_KNIGHT:
        case WHITE_BISHOP: case BLACK_BISHOP: return 1;
        case WHITE_ROOK: case BLACK_ROOK: return 2;
        case WHITE_QUEEN: case BLACK_QUEEN: return 4;
        default: return 0;
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

    Position knights[10];
    int knightCount = 0;

    Position bishopSquares[10];

    Position rooks[10];
    int rookCount = 0;

    Position queens[9];
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
        side.pawns[side.pawnCount++] = p;

        if (r < side.pawnMinRank[f]) side.pawnMinRank[f] = r;
        if (r > side.pawnMaxRank[f]) side.pawnMaxRank[f] = r;
    }
}

inline void collectPiecePositions(uint64_t bits, Position positions[], int& count) {
    while (bits != 0) {
        positions[count++] = Position(popLsbIndex(bits));
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

inline uint64_t kingZoneMask(Position king, bool blackKing) {
    if (king.isNone()) return 0;

    uint64_t mask = 0;
    int kr = king.rank();
    int kf = king.file();

    for (int dr = -1; dr <= 1; dr++) {
        for (int df = -1; df <= 1; df++) {
            int r = kr + dr;
            int f = kf + df;
            if (r >= 0 && r < 8 && f >= 0 && f < 8)
                mask |= 1ULL << (r * 8 + f);
        }
    }

    int front = blackKing ? -1 : 1;
    for (int df = -2; df <= 2; df++) {
        int r = kr + front;
        int f = kf + df;
        if (r >= 0 && r < 8 && f >= 0 && f < 8)
            mask |= 1ULL << (r * 8 + f);
    }

    return mask;
}

inline int countKnightKingZoneAttacks(const Board& board, Position from, bool attackerBlack, uint64_t zone) {
    int attacks = 0;
    int r = from.rank();
    int f = from.file();

    for (const auto& offset : knightOffsets) {
        int targetRank = r + offset[0];
        int targetFile = f + offset[1];
        if (!squareInMask(targetRank, targetFile, zone)) continue;

        Piece target = evalPieceAt(board, targetRank, targetFile);
        attacks += enemyColor(target, attackerBlack) ? 1 : 0;
    }

    return attacks;
}

inline int countPawnKingZoneAttacks(Position from, bool black, uint64_t zone) {
    int dir = black ? -1 : 1;
    int r = from.rank();
    int f = from.file();

    int attacks = 0;
    attacks += squareInMask(r + dir, f - 1, zone) ? 1 : 0;
    attacks += squareInMask(r + dir, f + 1, zone) ? 1 : 0;
    return attacks;
}

template <size_t N>
inline int countSlidingKingZoneAttacks(
    const Board& board,
    Position from,
    bool attackerBlack,
    const int (&directions)[N][2],
    uint64_t zone
) {
    int attacks = 0;

    int startRank = from.rank();
    int startFile = from.file();

    for (const auto& direction : directions) {
        int rank = startRank;
        int file = startFile;

        for (int distance = 1; distance < 8; distance++) {
            rank += direction[0];
            file += direction[1];
            if (rank < 0 || rank >= 8 || file < 0 || file >= 8) break;

            int idx = rank * 8 + file;
            Piece piece = evalPieceAt(board, rank, file);
            if ((zone & (1ULL << idx)) && enemyColor(piece, attackerBlack))
                attacks++;

            if (piece != EMPTY)
                break;
        }
    }

    return attacks;
}

// -----------------------------------------------------------------------
// Mobility: count pseudolegal moves (squares a piece can reach or attack).
// Doesn't do full legality check — fast approximation.
// Returns mobility count; caller multiplies by a per-piece bonus.
// -----------------------------------------------------------------------
inline int knightMobility(Position from, uint64_t ownPieces) {
    int r = from.rank();
    int f = from.file();
    int count = 0;

    for (const auto& o : knightOffsets) {
        int nr = r + o[0];
        int nf = f + o[1];
        if (nr < 0 || nr >= 8 || nf < 0 || nf >= 8) continue;
        int sq = nr * 8 + nf;
        if (!(ownPieces >> sq & 1)) count++;
    }
    return count;
}

inline int slidingMobility(Position from, uint64_t occ, uint64_t ownPieces,
                            const int dirs[][2], int numDirs)
{
    int r = from.rank();
    int f = from.file();
    int count = 0;

    for (int d = 0; d < numDirs; d++) {
        int nr = r, nf = f;
        for (int step = 1; step < 8; step++) {
            nr += dirs[d][0];
            nf += dirs[d][1];
            if (nr < 0 || nr >= 8 || nf < 0 || nf >= 8) break;
            int sq = nr * 8 + nf;
            if (ownPieces >> sq & 1) break; // blocked by own piece
            count++;
            if (occ >> sq & 1) break; // blocked by any piece (captures count)
        }
    }
    return count;
}

inline float evalMobilityCached(const Board& board, const EvalSideData& white, const EvalSideData& black, float midgameT, float endgameT) {
    // Build occupancy bitboards
    uint64_t whiteOcc = 0, blackOcc = 0;
    for (int p = 1; p <= 6; p++)  whiteOcc |= board.getPieceBits(static_cast<Piece>(p));
    for (int p = 9; p <= 14; p++) blackOcc |= board.getPieceBits(static_cast<Piece>(p));
    uint64_t allOcc = whiteOcc | blackOcc;

    // Mobility bonuses per extra move (in pawn units / 100)
    constexpr float KNIGHT_MOB_MG = 0.040f;
    constexpr float KNIGHT_MOB_EG = 0.040f;
    constexpr float BISHOP_MOB_MG = 0.038f;
    constexpr float BISHOP_MOB_EG = 0.038f;
    constexpr float ROOK_MOB_MG   = 0.022f;
    constexpr float ROOK_MOB_EG   = 0.038f;
    constexpr float QUEEN_MOB_MG  = 0.010f;
    constexpr float QUEEN_MOB_EG  = 0.018f;

    float wScore = 0.0f, bScore = 0.0f;

    // Knights
    for (int i = 0; i < white.knightCount; i++) {
        int mob = knightMobility(white.knights[i], whiteOcc);
        wScore += float(mob) * (KNIGHT_MOB_MG * midgameT + KNIGHT_MOB_EG * endgameT);
    }
    for (int i = 0; i < black.knightCount; i++) {
        int mob = knightMobility(black.knights[i], blackOcc);
        bScore += float(mob) * (KNIGHT_MOB_MG * midgameT + KNIGHT_MOB_EG * endgameT);
    }

    // Bishops
    for (int i = 0; i < white.bishops; i++) {
        int mob = slidingMobility(white.bishopSquares[i], allOcc, whiteOcc,
                                   bishopDirections, 4);
        wScore += float(mob) * (BISHOP_MOB_MG * midgameT + BISHOP_MOB_EG * endgameT);
    }
    for (int i = 0; i < black.bishops; i++) {
        int mob = slidingMobility(black.bishopSquares[i], allOcc, blackOcc,
                                   bishopDirections, 4);
        bScore += float(mob) * (BISHOP_MOB_MG * midgameT + BISHOP_MOB_EG * endgameT);
    }

    // Rooks
    for (int i = 0; i < white.rookCount; i++) {
        int mob = slidingMobility(white.rooks[i], allOcc, whiteOcc,
                                   rookDirections, 4);
        wScore += float(mob) * (ROOK_MOB_MG * midgameT + ROOK_MOB_EG * endgameT);
    }
    for (int i = 0; i < black.rookCount; i++) {
        int mob = slidingMobility(black.rooks[i], allOcc, blackOcc,
                                   rookDirections, 4);
        bScore += float(mob) * (ROOK_MOB_MG * midgameT + ROOK_MOB_EG * endgameT);
    }

    // Queens
    for (int i = 0; i < white.queenCount; i++) {
        int mob = slidingMobility(white.queens[i], allOcc, whiteOcc,
                                   queenDirections, 8);
        wScore += float(mob) * (QUEEN_MOB_MG * midgameT + QUEEN_MOB_EG * endgameT);
    }
    for (int i = 0; i < black.queenCount; i++) {
        int mob = slidingMobility(black.queens[i], allOcc, blackOcc,
                                   queenDirections, 8);
        bScore += float(mob) * (QUEEN_MOB_MG * midgameT + QUEEN_MOB_EG * endgameT);
    }

    return wScore - bScore; // positive = white advantage
}

inline float evalKingAttackCached(
    const Board& board,
    const EvalSideData& attacker,
    const EvalSideData& defender,
    bool attackerBlack
) {
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

    for (int i = 0; i < attacker.pawnCount; i++) {
        int attacks = countPawnKingZoneAttacks(attacker.pawns[i], attackerBlack, zone);
        score += 0.07f * float(attacks);
        attackUnits += attacks;
    }

    for (int i = 0; i < attacker.knightCount; i++) {
        addAttacks(countKnightKingZoneAttacks(board, attacker.knights[i], attackerBlack, zone), 2, 0.13f);
    }

    for (int i = 0; i < attacker.bishops; i++) {
        addAttacks(countSlidingKingZoneAttacks(board, attacker.bishopSquares[i], attackerBlack, bishopDirections, zone), 2, 0.11f);
    }

    for (int i = 0; i < attacker.rookCount; i++) {
        addAttacks(countSlidingKingZoneAttacks(board, attacker.rooks[i], attackerBlack, rookDirections, zone), 3, 0.16f);
    }

    for (int i = 0; i < attacker.queenCount; i++) {
        addAttacks(countSlidingKingZoneAttacks(board, attacker.queens[i], attackerBlack, queenDirections, zone), 4, 0.10f);
    }

    if (attackingPieces >= 2)
        score += 0.04f * float(attackUnits) + 0.10f * float(attackingPieces - 1);
    if (attacker.queenCount > 0 && attackingPieces >= 2)
        score += 0.12f;

    int kingFile = defender.kingPos.file();
    int kingRank = defender.kingPos.rank();
    for (int df = -1; df <= 1; df++) {
        int file = kingFile + df;
        if (file < 0 || file >= 8) continue;

        bool defenderPawn = defender.pawnFiles[file] > 0;
        bool attackerPawn = attacker.pawnFiles[file] > 0;
        bool shelterPawn = false;

        if (defenderPawn) {
            if (defenderBlack) {
                int closestPawn = defender.pawnMaxRank[file];
                shelterPawn = closestPawn < kingRank && closestPawn >= kingRank - 3;
            } else {
                int closestPawn = defender.pawnMinRank[file];
                shelterPawn = closestPawn > kingRank && closestPawn <= kingRank + 3;
            }
        }

        if (!defenderPawn && !attackerPawn)
            score += 0.16f;
        else if (!defenderPawn)
            score += 0.09f;

        if (!shelterPawn)
            score += df == 0 ? 0.10f : 0.06f;
    }

    return score;
}

inline float evalPawnStructureCached(const EvalSideData& side, const EvalSideData& enemy, bool black, float endgameT) {
    float score = 0.0f;

    for (int i = 0; i < side.pawnCount; i++) {
        Position p = side.pawns[i];
        int r = p.rank();
        int f = p.file();
        int advanceRank = black ? 7 - r : r;

        // Doubled pawns
        if (side.pawnFiles[f] > 1)
            score -= 0.12f * float(side.pawnFiles[f] - 1);

        // Isolated pawns
        bool isolated = !(side.pawnFileMask & (
            (f > 0 ? (1u << (f - 1)) : 0u) |
            (f < 7 ? (1u << (f + 1)) : 0u)
        ));
        if (isolated)
            score -= 0.16f;

        // Backward pawn: no friendly pawn behind or on same rank on adjacent files,
        // and the stop square is controlled by an enemy pawn.
        // Approximation: if isolated or only pawn on file and not advanced enough to support
        {
            bool backward = false;
            if (!isolated) {
                // Check if any friendly pawn is behind on an adjacent file
                int behindRank = black ? r + 1 : r - 1;
                bool hasSupportBehind = false;
                for (int df = -1; df <= 1; df += 2) {
                    int ff = f + df;
                    if (ff < 0 || ff >= 8) continue;
                    if (side.pawnFiles[ff] == 0) continue;
                    // Is there a pawn behind on this file?
                    if (!black && side.pawnMinRank[ff] < r) { hasSupportBehind = true; break; }
                    if ( black && side.pawnMaxRank[ff] > r) { hasSupportBehind = true; break; }
                }

                if (!hasSupportBehind) {
                    // Check if the stop square is attacked by an enemy pawn
                    int stopRank = black ? r - 1 : r + 1;
                    bool stopAttacked = false;
                    for (int df = -1; df <= 1; df += 2) {
                        int ff = f + df;
                        if (ff < 0 || ff >= 8) continue;
                        if (enemy.pawnFiles[ff] > 0) {
                            // Is an enemy pawn on this file at the right rank to attack the stop?
                            if (!black && enemy.pawnMaxRank[ff] == stopRank) { stopAttacked = true; break; }
                            if ( black && enemy.pawnMinRank[ff] == stopRank) { stopAttacked = true; break; }
                        }
                    }
                    if (stopAttacked) backward = true;
                }
            }
            if (backward) score -= 0.12f;
        }

        // Passed pawn
        bool passed = true;
        for (int df = -1; df <= 1 && passed; df++) {
            int ff = f + df;
            if (ff < 0 || ff >= 8) continue;
            if (enemy.pawnFiles[ff] == 0) continue;

            if (!black) {
                if (enemy.pawnMinRank[ff] > r)
                    passed = false;
            } else {
                if (enemy.pawnMaxRank[ff] < r)
                    passed = false;
            }
        }

        if (passed) {
            static constexpr float passedBonus[8] = {
                0.0f, 0.05f, 0.12f, 0.25f,
                0.45f, 0.75f, 1.25f, 0.0f
            };
            score += passedBonus[advanceRank] * (1.0f + 1.8f * endgameT);
            if (advanceRank >= 5)
                score += 0.18f * float(advanceRank - 4);
        }
    }

    return score;
}
inline float evalRooksCached(const EvalSideData& side, const EvalSideData& enemy, bool black) {
    float score = 0.0f;
    int seventh = black ? 1 : 6;

    for (int i = 0; i < side.rookCount; i++) {
        int f = side.rooks[i].file();
        bool ownPawnOnFile   = side.pawnFiles[f] > 0;
        bool enemyPawnOnFile = enemy.pawnFiles[f] > 0;

        if (!ownPawnOnFile && !enemyPawnOnFile) score += 0.30f;
        else if (!ownPawnOnFile)               score += 0.16f;

        if (side.rooks[i].rank() == seventh)   score += 0.22f;
    }

    return score;
}
inline float evalKingShieldCached(const Board& board, const EvalSideData& side, bool black) {
    if (!side.hasKing) return 0.0f;

    Piece pawn = black ? BLACK_PAWN : WHITE_PAWN;
    float score = 0.0f;

    int dir = black ? -1 : 1;
    int kr = side.kingPos.rank();
    int kf = side.kingPos.file();

    for (int df = -1; df <= 1; df++) {
        int r = kr + dir;
        int f = kf + df;

        if (r < 0 || r >= 8 || f < 0 || f >= 8)
            continue;

        if (evalPieceAt(board, r, f) == pawn)
            score += 0.10f;
        else
            score -= 0.08f;
    }

    return score;
}
inline float evalCenterCached(const Board& board, bool black) {
    float score = 0.0f;

    constexpr int centers[4] = {
        3 * 8 + 3,
        3 * 8 + 4,
        4 * 8 + 3,
        4 * 8 + 4
    };

    for (int idx : centers) {
        Piece piece = evalPieceAt(board, idx >> 3, idx & 7);

        if (sameColor(piece, black))
            score += 0.12f;
    }

    return score;
}
inline float endgameKingActivity(Position king) {
    int centerDist =
        std::min({
            manhattan(king, Position(3, 3)),
            manhattan(king, Position(3, 4)),
            manhattan(king, Position(4, 3)),
            manhattan(king, Position(4, 4))
        });

    return -0.10f * float(centerDist);
}
inline float kingSupportsPawns(const EvalSideData& side, const EvalSideData& enemy, bool black) {
    float score = 0.0f;

    for (int i = 0; i < side.pawnCount; i++) {
        Position pawn = side.pawns[i];

        int promotionRank = black ? 0 : 7;
        Position promo(promotionRank, pawn.file());

        int ownKingDist = manhattan(side.kingPos, pawn);
        int enemyKingDist = manhattan(enemy.kingPos, promo);

        score += 0.06f * float(6 - std::min(6, ownKingDist));
        score += 0.08f * float(enemyKingDist);
    }

    return score;
}
inline float evalCastlingStatus(const Board& board, const EvalSideData& side, bool black) {
    if (!side.hasKing) return 0.0f;

    Position kingSideCastle = black ? Position{7, 6} : Position{0, 6};
    Position queenSideCastle = black ? Position{7, 2} : Position{0, 2};

    if (side.kingPos == kingSideCastle || side.kingPos == queenSideCastle) {
        return 0.55f;
    }

    float score = 0.0f;
    if (board.canCastleKingSide(black)) score += 0.14f;
    if (board.canCastleQueenSide(black)) score += 0.12f;

    return score;
}

// -----------------------------------------------------------------------
// Threat detection: bonus for attacking undefended enemy pieces with
// a lower-value piece. Uses simple presence check — fast.
// -----------------------------------------------------------------------
inline float evalThreats(const Board& board, const EvalSideData& us, const EvalSideData& them, bool usBlack) {
    float score = 0.0f;

    // Knight threats to non-pawn enemy pieces
    Piece enemyPawn   = usBlack ? WHITE_PAWN   : BLACK_PAWN;
    for (int i = 0; i < us.knightCount; i++) {
        int r = us.knights[i].rank();
        int f = us.knights[i].file();
        for (const auto& o : knightOffsets) {
            int nr = r + o[0], nf = f + o[1];
            if (nr < 0 || nr >= 8 || nf < 0 || nf >= 8) continue;
            Piece target = evalPieceAt(board, nr, nf);
            if (!enemyColor(target, usBlack)) continue;
            if (target == enemyPawn) continue;
            // Knight attacks a non-pawn enemy piece — minor threat bonus
            score += 0.06f;
        }
    }

    // Pawn threats to enemy minor/major pieces
    int dir = usBlack ? -1 : 1;
    for (int i = 0; i < us.pawnCount; i++) {
        int r = us.pawns[i].rank();
        int f = us.pawns[i].file();
        for (int df : {-1, 1}) {
            int nr = r + dir, nf = f + df;
            if (nr < 0 || nr >= 8 || nf < 0 || nf >= 8) continue;
            Piece target = evalPieceAt(board, nr, nf);
            if (!enemyColor(target, usBlack)) continue;
            if (target == enemyPawn) continue;
            // Pawn attacks a piece — strong threat
            score += 0.10f;
        }
    }

    return score;
}

[[nodiscard]] inline float evaluateBoard(const Board& board) {
    EvalSideData white;
    EvalSideData black;

    collectPawnData(board.getPieceBits(WHITE_PAWN), white);
    collectPawnData(board.getPieceBits(BLACK_PAWN), black);

    collectPiecePositions(board.getPieceBits(WHITE_ROOK), white.rooks, white.rookCount);
    collectPiecePositions(board.getPieceBits(BLACK_ROOK), black.rooks, black.rookCount);

    collectPiecePositions(board.getPieceBits(WHITE_KNIGHT), white.knights, white.knightCount);
    collectPiecePositions(board.getPieceBits(BLACK_KNIGHT), black.knights, black.knightCount);

    collectPiecePositions(board.getPieceBits(WHITE_BISHOP), white.bishopSquares, white.bishops);
    collectPiecePositions(board.getPieceBits(BLACK_BISHOP), black.bishopSquares, black.bishops);

    collectPiecePositions(board.getPieceBits(WHITE_QUEEN), white.queens, white.queenCount);
    collectPiecePositions(board.getPieceBits(BLACK_QUEEN), black.queens, black.queenCount);

    collectKingPosition(board.getPieceBits(WHITE_KING), white);
    collectKingPosition(board.getPieceBits(BLACK_KING), black);

    int phase = board.getEvalPhase();
    float endgameT = 1.0f - std::min(24.0f, float(phase)) / 24.0f;
    float midgameT = 1.0f - endgameT;

    float whiteScore = board.getWhiteMg() * midgameT + board.getWhiteEg() * endgameT;
    float blackScore = board.getBlackMg() * midgameT + board.getBlackEg() * endgameT;

    whiteScore += evalPawnStructureCached(white, black, false, endgameT);
    blackScore += evalPawnStructureCached(black, white, true, endgameT);

    whiteScore += evalRooksCached(white, black, false);
    blackScore += evalRooksCached(black, white, true);

    if (white.bishops >= 2) whiteScore += 0.35f;
    if (black.bishops >= 2) blackScore += 0.35f;

    whiteScore += evalCenterCached(board, false) * midgameT;
    blackScore += evalCenterCached(board, true) * midgameT;

    whiteScore += evalKingShieldCached(board, white, false) * midgameT;
    blackScore += evalKingShieldCached(board, black, true) * midgameT;

    whiteScore += evalCastlingStatus(board, white, false) * midgameT;
    blackScore += evalCastlingStatus(board, black, true) * midgameT;

    whiteScore += evalKingAttackCached(board, white, black, false) * midgameT;
    blackScore += evalKingAttackCached(board, black, white, true) * midgameT;

    whiteScore += endgameKingActivity(white.kingPos) * endgameT;
    blackScore += endgameKingActivity(black.kingPos) * endgameT;

    whiteScore += kingSupportsPawns(white, black, false) * endgameT;
    blackScore += kingSupportsPawns(black, white, true) * endgameT;

    // Mobility (both phases)
    float mobilityDiff = evalMobilityCached(board, white, black, midgameT, endgameT);
    whiteScore += mobilityDiff > 0.0f ?  mobilityDiff : 0.0f;
    blackScore += mobilityDiff < 0.0f ? -mobilityDiff : 0.0f;

    // Threats
    whiteScore += evalThreats(board, white, black, false);
    blackScore += evalThreats(board, black, white, true);

    float eval = whiteScore - blackScore;

    return board.getPlayerTurn() ? -eval : eval;
}

#endif
