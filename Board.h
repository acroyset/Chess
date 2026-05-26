//
// Created by Andreas Royset on 11/30/24.
//

#ifndef BOARD_H
#define BOARD_H
#include "BoardState.h"
#include "AI/OpeningBook.h"

inline float tableMg(Piece p, int idx);
inline float tableEg(Piece p, int idx);
inline int gamePhaseWeight(Piece p);

constexpr int knightOffsets[8][2] = {
    { 2,  1}, { 2, -1},
    {-2,  1}, {-2, -1},
    { 1,  2}, { 1, -2},
    {-1,  2}, {-1, -2}
};
constexpr int bishopDirections[4][2] = {
    { 1,  1},
    { 1, -1},
    {-1,  1},
    {-1, -1}
};
constexpr int rookDirections[4][2] = {
    { 1,  0},
    {-1,  0},
    { 0,  1},
    { 0, -1}
};
constexpr int queenDirections[8][2] = {
    { 1,  0}, {-1,  0},
    { 0,  1}, { 0, -1},
    { 1,  1}, { 1, -1},
    {-1,  1}, {-1, -1}
};
constexpr int kingOffsets[8][2] = {
    {-1, -1}, {0, -1}, {1, -1},
    {-1, 0},              {1, 0},
    {-1, 1},  {0, 1},  {1, 1}
};

inline uint64_t zobristPieces[16][64];
inline uint64_t zobristBlackTurn;
inline uint64_t zobristCastle[16];
inline uint64_t zobristEnPassant[8];

inline bool zobristInitialized = false;

inline void initZobrist() {
    if (zobristInitialized) return;
    zobristInitialized = true;
    std::mt19937_64 rng(0x123456789ABCDEF);

    auto rand64 = [&]() {
        return rng();
    };

    for (auto & zobristPiece : zobristPieces) {
        for (unsigned long long & square : zobristPiece) {
            square = rand64();
        }
    }

    for (unsigned long long & i : zobristCastle) {
        i = rand64();
    }

    for (unsigned long long & i : zobristEnPassant) {
        i = rand64();
    }

    zobristBlackTurn = rand64();
}

// Maximum game length for fixed-size history stack
// 512 is more than enough for any real game + search depth
static constexpr int MAX_HISTORY = 512;

class Board{
    BoardState state;

    // Fixed-size stack — eliminates all heap allocation during search
    BoardState stateStack[MAX_HISTORY];
    int stackTop = 0;

    // Separate vector only for repetition detection across the root
    // (search threads share the stack; root history is small)
    std::vector<uint64_t> rootHashes;

public:

    Board() {
        initZobrist();
        resetBoard();
        refreshPieceBits();
        state.hash = computeHash();
        refreshIncrementalEval();
    }
    explicit Board(const std::string& fen) {
        initZobrist();

        state = BoardState();
        stackTop = 0;

        std::stringstream ss(fen);

        std::string boardPart;
        std::string turnPart;
        std::string castlePart;
        std::string epPart;
        int halfmove = 0;
        int fullmove = 1;

        ss >> boardPart >> turnPart >> castlePart >> epPart >> halfmove >> fullmove;

        for (int i = 0; i < 8; i++) {
            state.board[i] = 0;
        }

        int rank = 7;
        int file = 0;

        for (char c : boardPart) {
            if (c == '/') {
                rank--;
                file = 0;
                continue;
            }

            if (std::isdigit(c)) {
                file += c - '0';
                continue;
            }

            Piece piece = EMPTY;

            switch (c) {
                case 'P': piece = WHITE_PAWN; break;
                case 'N': piece = WHITE_KNIGHT; break;
                case 'B': piece = WHITE_BISHOP; break;
                case 'R': piece = WHITE_ROOK; break;
                case 'Q': piece = WHITE_QUEEN; break;
                case 'K': piece = WHITE_KING; break;

                case 'p': piece = BLACK_PAWN; break;
                case 'n': piece = BLACK_KNIGHT; break;
                case 'b': piece = BLACK_BISHOP; break;
                case 'r': piece = BLACK_ROOK; break;
                case 'q': piece = BLACK_QUEEN; break;
                case 'k': piece = BLACK_KING; break;

                default: break;
            }

            set({rank, file}, piece);
            file++;
        }

        state.playerTurn = turnPart == "b";

        state.whiteCastleKing  = castlePart.find('K') != std::string::npos;
        state.whiteCastleQueen = castlePart.find('Q') != std::string::npos;
        state.blackCastleKing  = castlePart.find('k') != std::string::npos;
        state.blackCastleQueen = castlePart.find('q') != std::string::npos;

        state.lastPawnMoved2.setNone();

        if (epPart != "-") {
            int epFile = epPart[0] - 'a';
            int epRank = epPart[1] - '1';

            if (state.playerTurn == WHITE) {
                state.lastPawnMoved2 = {epRank + 1, epFile};
            } else {
                state.lastPawnMoved2 = {epRank - 1, epFile};
            }
        }

        state.lastPawnMoveOrCapture = halfmove;

        state.whiteKing = findKing(false);
        state.blackKing = findKing(true);

        refreshPieceBits();
        state.hash = computeHash();
        refreshIncrementalEval();
    }
    Board(const Board& other) {
        state    = other.state;
        stackTop = other.stackTop;
        rootHashes = other.rootHashes;
        for (int i = 0; i < stackTop; i++)
            stateStack[i] = other.stateStack[i];
        refreshPieceBits();
        state.hash = computeHash();
        refreshIncrementalEval();
    }
    void set(const Board &other) {
        this->state    = other.state;
        this->stackTop = other.stackTop;
        this->rootHashes = other.rootHashes;
        for (int i = 0; i < stackTop; i++)
            stateStack[i] = other.stateStack[i];
        refreshPieceBits();
        state.hash = computeHash();
        refreshIncrementalEval();
    }

    [[nodiscard]] Piece getPiece(const Position position) const {
        if (position.isNone()) return EMPTY;

        uint8_t bits = state.board[position.rank()] >> (7-position.file())*4 & 0xF;

        return static_cast<Piece>(bits);
    }
    [[nodiscard]] PieceType getPieceType(const Position position) const {
        if (position.isNone()) return NONE;

        const uint8_t bits = state.board[position.rank()] >> (7-position.file())*4 & 0xF;

        return bits == 0 ? NONE : bits >> 3 ? BLACK : WHITE;
    }
    [[nodiscard]] bool getPlayerTurn() const {
        return state.playerTurn;
    }
    [[nodiscard]] bool canCastleKingSide(bool black) const {
        return black ? state.blackCastleKing : state.whiteCastleKing;
    }
    [[nodiscard]] bool canCastleQueenSide(bool black) const {
        return black ? state.blackCastleQueen : state.whiteCastleQueen;
    }
    [[nodiscard]] float getWhiteMg() const {
        return state.whiteMg;
    }
    [[nodiscard]] float getWhiteEg() const {
        return state.whiteEg;
    }
    [[nodiscard]] float getBlackMg() const {
        return state.blackMg;
    }
    [[nodiscard]] float getBlackEg() const {
        return state.blackEg;
    }
    [[nodiscard]] int getEvalPhase() const {
        return state.evalPhase;
    }
    [[nodiscard]] uint32_t getRankBits(int rank) const {
        return state.board[rank];
    }
    [[nodiscard]] uint64_t getPieceBits(Piece piece) const {
        return state.pieceBits[piece];
    }

    // --- Move generation using pieceBits iteration ---

    void getValidMoves(MoveList& validMoves) {
        validMoves.clear();

        MoveList possibleMoves;

        // Iterate over only occupied squares via pieceBits instead of all 64
        uint64_t friendly = getFriendlyBits();
        while (friendly) {
            int sq = popLsb(friendly);
            Position position(sq);

            possibleMoves.clear();
            getPossibleMoves(position, possibleMoves);

            bool inCheck = check(state.playerTurn);
            Position kingPos = state.playerTurn ? state.blackKing : state.whiteKing;

            for (int i = 0; i < possibleMoves.count; i++) {
                Move move = possibleMoves[i];

                // Fast rejection: if not in check and the piece isn't on a king ray,
                // only castling moves need full validation
                if (!inCheck && !move.isCastleKing() && !move.isCastleQueen()) {
                    // Check if moving piece is on same rank/file/diagonal as king
                    // (potential pin) — only then do full validation
                    Position from = move.starting();
                    bool couldBePin = false;
                    int dr = int(from.rank()) - int(kingPos.rank());
                    int df = int(from.file()) - int(kingPos.file());
                    // On same rank, file, or diagonal
                    if (dr == 0 || df == 0 || std::abs(dr) == std::abs(df))
                        couldBePin = true;

                    if (!couldBePin) {
                        validMoves.push(move);
                        continue;
                    }
                }

                if (validMove(possibleMoves[i])) {
                    validMoves.push(possibleMoves[i]);
                }
            }
        }
    }
    void getCaptureMoves(MoveList& validMoves) {
        validMoves.clear();

        MoveList captureMoves;

        uint64_t friendly = getFriendlyBits();
        while (friendly) {
            int sq = popLsb(friendly);
            Position position(sq);

            captureMoves.clear();
            getPossibleCaptures(position, captureMoves);

            bool inCheck = check(state.playerTurn);
            Position kingPos = state.playerTurn ? state.blackKing : state.whiteKing;

            for (int i = 0; i < captureMoves.count; i++) {
                Move move = captureMoves[i];

                // Fast rejection: if not in check and the piece isn't on a king ray,
                // only castling moves need full validation
                if (!inCheck && !move.isCastleKing() && !move.isCastleQueen()) {
                    // Check if moving piece is on same rank/file/diagonal as king
                    // (potential pin) — only then do full validation
                    Position from = move.starting();
                    bool couldBePin = false;
                    int dr = int(from.rank()) - int(kingPos.rank());
                    int df = int(from.file()) - int(kingPos.file());
                    // On same rank, file, or diagonal
                    if (dr == 0 || df == 0 || std::abs(dr) == std::abs(df))
                        couldBePin = true;

                    if (!couldBePin) {
                        validMoves.push(move);
                        continue;
                    }
                }

                if (validMove(captureMoves[i])) {
                    validMoves.push(captureMoves[i]);
                }
            }
        }
    }
    void getValidMoves(const Position position, MoveList& validMoves) {
        validMoves.clear();

        MoveList possibleMoves;

        getPossibleMoves(position, possibleMoves);

        bool inCheck = check(state.playerTurn);
        Position kingPos = state.playerTurn ? state.blackKing : state.whiteKing;

        for (int i = 0; i < possibleMoves.count; i++) {
            Move move = possibleMoves[i];

            // Fast rejection: if not in check and the piece isn't on a king ray,
            // only castling moves need full validation
            if (!inCheck && !move.isCastleKing() && !move.isCastleQueen()) {
                // Check if moving piece is on same rank/file/diagonal as king
                // (potential pin) — only then do full validation
                Position from = move.starting();
                bool couldBePin = false;
                int dr = int(from.rank()) - int(kingPos.rank());
                int df = int(from.file()) - int(kingPos.file());
                // On same rank, file, or diagonal
                if (dr == 0 || df == 0 || std::abs(dr) == std::abs(df))
                    couldBePin = true;

                if (!couldBePin) {
                    validMoves.push(move);
                    continue;
                }
            }

            if (validMove(move)) {
                validMoves.push(move);
            }
        }
    }

    [[nodiscard]] bool check(const bool player) const {
        Position kingPosition = player ? state.blackKing : state.whiteKing;

        if (!kingPosition.isNone()) {
            return squareAttacked(kingPosition, !player);
        }

        std::cerr << "No King"<< std::endl;
        return false;
    }
    bool moveGivesCheck(Move move) const {
        Position target = move.target();
        Piece piece = getPiece(move.starting());
        bool black = piece >> 3;
        Position enemyKing = black ? state.whiteKing : state.blackKing;

        if (enemyKing.isNone()) return false;

        int kr = enemyKing.rank(), kf = enemyKing.file();
        int tr = target.rank(),    tf = target.file();

        switch (piece & 7) {
            case 1: { // pawn
                int dir = black ? -1 : 1;
                return (tr == kr + dir) && (tf == kf - 1 || tf == kf + 1);
            }
            case 2: { // knight
                int dr = std::abs(tr - kr), df = std::abs(tf - kf);
                return (dr == 2 && df == 1) || (dr == 1 && df == 2);
            }
            case 3: { // bishop — check if target is on same diagonal as enemy king
                int dr = std::abs(tr - kr), df = std::abs(tf - kf);
                if (dr != df) return false;
                // Verify no pieces between target and king
                int sr = (kr > tr) ? 1 : -1, sf = (kf > tf) ? 1 : -1;
                int r = tr + sr, f = tf + sf;
                while (r != kr || f != kf) {
                    if (getPiece(Position(r, f)) != EMPTY) return false;
                    r += sr; f += sf;
                }
                return true;
            }
            case 4: { // rook
                if (tr != kr && tf != kf) return false;
                int sr = (tr == kr) ? 0 : (kr > tr ? 1 : -1);
                int sf = (tf == kf) ? 0 : (kf > tf ? 1 : -1);
                int r = tr + sr, f = tf + sf;
                while (r != kr || f != kf) {
                    if (getPiece(Position(r, f)) != EMPTY) return false;
                    r += sr; f += sf;
                }
                return true;
            }
            case 5: { // queen — bishop + rook combined
                int dr = std::abs(tr - kr), df = std::abs(tf - kf);
                bool diagonal   = (dr == df);
                bool orthogonal = (tr == kr || tf == kf);
                if (!diagonal && !orthogonal) return false;
                int sr = (kr == tr) ? 0 : (kr > tr ? 1 : -1);
                int sf = (kf == tf) ? 0 : (kf > tf ? 1 : -1);
                int r = tr + sr, f = tf + sf;
                while (r != kr || f != kf) {
                    if (getPiece(Position(r, f)) != EMPTY) return false;
                    r += sr; f += sf;
                }
                return true;
            }
            default: return false; // king moves — ignore
        }
    }
    [[nodiscard]] bool checkMate() {
        return noMoves(state.playerTurn) && check(state.playerTurn);
    }
    [[nodiscard]] bool draw() {
        if (draw50Move()) return true;
        if (drawRepetition()) return true;
        if (drawInsufficientMaterial()) return true;
        if (staleMate(state.playerTurn)) return true;

        return false;
    }
    [[nodiscard]] bool draw50Move() const {
        return state.lastPawnMoveOrCapture >= 100;
    }
    [[nodiscard]] bool drawInsufficientMaterial() const {
        const uint64_t pawns =
            state.pieceBits[WHITE_PAWN] | state.pieceBits[BLACK_PAWN];
        const uint64_t rooks =
            state.pieceBits[WHITE_ROOK] | state.pieceBits[BLACK_ROOK];
        const uint64_t queens =
            state.pieceBits[WHITE_QUEEN] | state.pieceBits[BLACK_QUEEN];

        if (pawns || rooks || queens) return false;

        const uint64_t knights =
            state.pieceBits[WHITE_KNIGHT] | state.pieceBits[BLACK_KNIGHT];
        const uint64_t bishops =
            state.pieceBits[WHITE_BISHOP] | state.pieceBits[BLACK_BISHOP];
        const uint64_t minors = knights | bishops;

        if (!minors) return true;

        if (knights) {
            return __builtin_popcountll(minors) == 1;
        }

        bool hasLightBishop = false;
        bool hasDarkBishop = false;
        uint64_t bishopBits = bishops;

        while (bishopBits) {
            const int square = popLsb(bishopBits);
            const bool lightSquare = ((square / 8) + (square % 8)) & 1;
            hasLightBishop = hasLightBishop || lightSquare;
            hasDarkBishop = hasDarkBishop || !lightSquare;
        }

        return !(hasLightBishop && hasDarkBishop);
    }
    [[nodiscard]] bool drawRepetition() const {
        uint64_t current = state.hash;
        int count = 0;

        // Check stack (search history)
        for (int i = stackTop - 1; i >= 0; i--) {
            if (stateStack[i].hash == current && ++count >= 2) return true;
        }
        // Check root history (game history before search started)
        for (auto h : rootHashes) {
            if (h == current && ++count >= 2) return true;
        }
        return false;
    }
    [[nodiscard]] bool repetition() const {
        uint64_t current = state.hash;

        // Check stack (search history)
        for (int i = stackTop - 1; i >= 0; i--) {
            if (stateStack[i].hash == current) return true;
        }
        // Check root history (game history before search started)
        return std::any_of(rootHashes.begin(), rootHashes.end(), [current](const uint64_t& h){return h == current;});
    }

    bool move(const Move& move) {

        Piece piece = getPiece(move.starting());
        if (piece == EMPTY) {
            std::cerr << "Empty Piece, Flag:" << std::bitset<4>(move.getData() >> 12) << " S:" << int(move.starting().index) << " T:" << int(move.target().index) << std::endl;
            std::vector<int> x;
            std::cout << x[0] << std::endl;
            return false;
        }
        if (piece >> 3 != state.playerTurn) {
            std::cerr << "Wrong color, Flag:" << std::bitset<4>(move.getData() >> 12) << " S:" << int(move.starting().index) << " T:" << int(move.target().index) << std::endl;
            std::vector<int> x;
            std::cout << x[0] << std::endl;
            return false;
        }

        // Push onto fixed-size stack — zero heap allocation
        stateStack[stackTop++] = state;

        removePieceFromEval(piece, move.starting());

        state.lastPawnMoveOrCapture++;
        if (move.isCapture()) state.lastPawnMoveOrCapture = 0;

        state.hash ^= zobristPieces[piece][move.starting().index];

        if (!state.lastPawnMoved2.isNone())
            state.hash ^= zobristEnPassant[state.lastPawnMoved2.file()];
        state.lastPawnMoved2.setNone();

        int oldCastleIndex = 0;

        if (state.whiteCastleKing)  oldCastleIndex |= 1;
        if (state.whiteCastleQueen) oldCastleIndex |= 2;
        if (state.blackCastleKing)  oldCastleIndex |= 4;
        if (state.blackCastleQueen) oldCastleIndex |= 8;

        switch (piece) {

            case WHITE_PAWN: {
                state.lastPawnMoveOrCapture = 0;

                if (abs(move.starting().rank() - move.target().rank()) == 2) {
                    state.lastPawnMoved2 = move.target();
                    state.hash ^= zobristEnPassant[state.lastPawnMoved2.file()];
                }

                if (move.isEnPassant()) {
                    Position capturedPawn{move.target().rank() - 1, move.target().file()};
                    state.hash ^= zobristPieces[BLACK_PAWN][capturedPawn.index];
                    removePieceFromEval(BLACK_PAWN, capturedPawn);
                    clear(capturedPawn);
                }

                if (move.isPromotion()) {
                    switch (move.promotionPiece()) {
                        case 0: {piece = WHITE_QUEEN; break;}
                        case 1: {piece = WHITE_ROOK; break;}
                        case 2: {piece = WHITE_BISHOP; break;}
                        case 3: {piece = WHITE_KNIGHT; break;}
                        default:
                    }
                }
                break;
            }

            case BLACK_PAWN: {
                state.lastPawnMoveOrCapture = 0;

                if (abs(move.starting().rank() - move.target().rank()) == 2) {
                    state.lastPawnMoved2 = move.target();
                    state.hash ^= zobristEnPassant[state.lastPawnMoved2.file()];
                }

                if (move.isEnPassant()) {
                    Position capturedPawn{move.target().rank() + 1, move.target().file()};
                    state.hash ^= zobristPieces[WHITE_PAWN][capturedPawn.index];
                    removePieceFromEval(WHITE_PAWN, capturedPawn);
                    clear(capturedPawn);
                }

                if (move.isPromotion()) {
                    switch (move.promotionPiece()) {
                        case 0: {piece = BLACK_QUEEN; break;}
                        case 1: {piece = BLACK_ROOK; break;}
                        case 2: {piece = BLACK_BISHOP; break;}
                        case 3: {piece = BLACK_KNIGHT; break;}
                        default:
                    }
                }
                break;
            }

            case WHITE_KING: {

                if (move.isCastleQueen()) {
                    removePieceFromEval(WHITE_ROOK, {0, 0});
                    movePiece({0, 0}, {0, 3}, WHITE_ROOK);
                    addPieceToEval(WHITE_ROOK, {0, 3});
                    state.hash ^= zobristPieces[WHITE_ROOK][0];
                    state.hash ^= zobristPieces[WHITE_ROOK][3];
                }
                if (move.isCastleKing()) {
                    removePieceFromEval(WHITE_ROOK, {0, 7});
                    movePiece({0, 7}, {0, 5}, WHITE_ROOK);
                    addPieceToEval(WHITE_ROOK, {0, 5});
                    state.hash ^= zobristPieces[WHITE_ROOK][7];
                    state.hash ^= zobristPieces[WHITE_ROOK][5];
                }

                state.whiteCastleKing = false;
                state.whiteCastleQueen = false;

                state.whiteKing = move.target();

                break;
            }

            case WHITE_ROOK: {
                if (move.starting() == Position{0,0}) state.whiteCastleQueen = false;
                if (move.starting() == Position{0,7}) state.whiteCastleKing = false;

                break;
            }

            case BLACK_KING: {

                if (move.isCastleQueen()) {
                    removePieceFromEval(BLACK_ROOK, {7, 0});
                    movePiece({7, 0}, {7, 3}, BLACK_ROOK);
                    addPieceToEval(BLACK_ROOK, {7, 3});
                    state.hash ^= zobristPieces[BLACK_ROOK][56];
                    state.hash ^= zobristPieces[BLACK_ROOK][59];
                }
                if (move.isCastleKing()) {
                    removePieceFromEval(BLACK_ROOK, {7, 7});
                    movePiece({7, 7}, {7, 5}, BLACK_ROOK);
                    addPieceToEval(BLACK_ROOK, {7, 5});
                    state.hash ^= zobristPieces[BLACK_ROOK][63];
                    state.hash ^= zobristPieces[BLACK_ROOK][61];
                }

                state.blackCastleKing = false;
                state.blackCastleQueen = false;

                state.blackKing = move.target();

                break;
            }

            case BLACK_ROOK: {

                if (move.starting() == Position{7,0}) state.blackCastleQueen = false;
                if (move.starting() == Position{7,7}) state.blackCastleKing = false;

                break;
            }

            default:
        }

        if (move.isCapture() && !move.isEnPassant()) {
            Piece capturedPiece = getPiece(move.target());
            state.hash ^= zobristPieces[capturedPiece][move.target().index];
            removePieceFromEval(capturedPiece, move.target());

            if (move.target() == Position{0,0}) state.whiteCastleQueen = false;
            if (move.target() == Position{0,7}) state.whiteCastleKing = false;
            if (move.target() == Position{7,0}) state.blackCastleQueen = false;
            if (move.target() == Position{7,7}) state.blackCastleKing = false;
        }

        int newCastleIndex = 0;
        if (state.whiteCastleKing)  newCastleIndex |= 1;
        if (state.whiteCastleQueen) newCastleIndex |= 2;
        if (state.blackCastleKing)  newCastleIndex |= 4;
        if (state.blackCastleQueen) newCastleIndex |= 8;

        state.hash ^= zobristCastle[oldCastleIndex];
        state.hash ^= zobristCastle[newCastleIndex];

        movePiece(move.starting(), move.target(), piece);
        addPieceToEval(piece, move.target());

        state.hash ^= zobristBlackTurn;
        state.hash ^= zobristPieces[piece][move.target().index];

        switchPlayer();

        return true;
    }
    void undoMove() {
        if (stackTop == 0) return;
        state = stateStack[--stackTop];
    }

    // Null move: flip side to move, clear EP, update hash
    // Used by null move pruning in search
    void makeNullMove() {
        stateStack[stackTop++] = state;

        if (!state.lastPawnMoved2.isNone()) {
            state.hash ^= zobristEnPassant[state.lastPawnMoved2.file()];
            state.lastPawnMoved2.setNone();
        }

        state.hash ^= zobristBlackTurn;
        state.playerTurn = !state.playerTurn;
        state.lastPawnMoveOrCapture++; // avoid 50-move false draws in search
    }
    void undoNullMove() {
        if (stackTop == 0) return;
        state = stateStack[--stackTop];
    }

    // Called at root before search starts — records game history for repetition detection
    void setRootHashes(const std::vector<uint64_t>& hashes) {
        rootHashes = hashes;
    }

    [[nodiscard]] uint64_t computeHash() const {
        uint64_t hash = 0;

        for (Position position; position <= Position(7, 7); position++) {
            Piece piece = getPiece(position);

            if (piece == EMPTY)
                continue;

            hash ^= zobristPieces[piece][position.index];
        }

        if (getPlayerTurn()) {
            hash ^= zobristBlackTurn;
        }

        int castleIndex = 0;

        if (state.whiteCastleKing)  castleIndex |= 1;
        if (state.whiteCastleQueen) castleIndex |= 2;
        if (state.blackCastleKing)  castleIndex |= 4;
        if (state.blackCastleQueen) castleIndex |= 8;

        hash ^= zobristCastle[castleIndex];

        if (!state.lastPawnMoved2.isNone()) {
            hash ^= zobristEnPassant[state.lastPawnMoved2.file()];
        }

        return hash;
    }
    [[nodiscard]] uint64_t getHash() const {
        return state.hash;
    }

    [[nodiscard]] uint64_t computePolyglotHash() const {
    uint64_t hash = 0;

    for (Position p; p <= Position(7,7); p++) {

        Piece piece = getPiece(p);

        if (piece == EMPTY)
            continue;

        int polyPiece = -1;

        switch (piece) {
            case BLACK_PAWN:   polyPiece = 0; break;
            case WHITE_PAWN:   polyPiece = 1; break;
            case BLACK_KNIGHT: polyPiece = 2; break;
            case WHITE_KNIGHT: polyPiece = 3; break;
            case BLACK_BISHOP: polyPiece = 4; break;
            case WHITE_BISHOP: polyPiece = 5; break;
            case BLACK_ROOK:   polyPiece = 6; break;
            case WHITE_ROOK:   polyPiece = 7; break;
            case BLACK_QUEEN:  polyPiece = 8; break;
            case WHITE_QUEEN:  polyPiece = 9; break;
            case BLACK_KING:   polyPiece = 10; break;
            case WHITE_KING:   polyPiece = 11; break;
            default: break;
        }

        int square = p.rank() * 8 + p.file();

        hash ^= polyglotRandom[64 * polyPiece + square];
    }

    if (state.whiteCastleKing)
        hash ^= polyglotRandom[768];

    if (state.whiteCastleQueen)
        hash ^= polyglotRandom[769];

    if (state.blackCastleKing)
        hash ^= polyglotRandom[770];

    if (state.blackCastleQueen)
        hash ^= polyglotRandom[771];

    if (!state.lastPawnMoved2.isNone()) {

        int file = state.lastPawnMoved2.file();
        int rank = state.lastPawnMoved2.rank();

        bool includeEP = false;

        if (!state.playerTurn) {

            if (getPiece({rank, file - 1}) == WHITE_PAWN)
                includeEP = true;

            if (getPiece({rank, file + 1}) == WHITE_PAWN)
                includeEP = true;
        }
        else {

            if (getPiece({rank, file - 1}) == BLACK_PAWN)
                includeEP = true;

            if (getPiece({rank, file + 1}) == BLACK_PAWN)
                includeEP = true;
        }

        if (includeEP)
            hash ^= polyglotRandom[772 + file];
    }

    if (!state.playerTurn)
        hash ^= polyglotRandom[780];

    return hash;
}

private:

    static int popLsb(uint64_t& bits) {
        int sq = __builtin_ctzll(bits);
        bits &= bits - 1;
        return sq;
    }

    [[nodiscard]] uint64_t getFriendlyBits() const {
        if (!state.playerTurn) {
            // White pieces: types 1-6
            return state.pieceBits[WHITE_PAWN]   | state.pieceBits[WHITE_KNIGHT] |
                   state.pieceBits[WHITE_BISHOP] | state.pieceBits[WHITE_ROOK]   |
                   state.pieceBits[WHITE_QUEEN]  | state.pieceBits[WHITE_KING];
        } else {
            // Black pieces: types 9-14
            return state.pieceBits[BLACK_PAWN]   | state.pieceBits[BLACK_KNIGHT] |
                   state.pieceBits[BLACK_BISHOP] | state.pieceBits[BLACK_ROOK]   |
                   state.pieceBits[BLACK_QUEEN]  | state.pieceBits[BLACK_KING];
        }
    }

    void clearIncrementalEval() {
        state.whiteMg = 0.0f;
        state.whiteEg = 0.0f;
        state.blackMg = 0.0f;
        state.blackEg = 0.0f;
        state.evalPhase = 0;
    }
    void addPieceToEval(Piece piece, Position position) {
        if (piece == EMPTY || position.isNone()) return;

        if (piece >> 3) {
            state.blackMg += tableMg(piece, position.index);
            state.blackEg += tableEg(piece, position.index);
        } else {
            state.whiteMg += tableMg(piece, position.index);
            state.whiteEg += tableEg(piece, position.index);
        }

        state.evalPhase += gamePhaseWeight(piece);
    }
    void removePieceFromEval(Piece piece, Position position) {
        if (piece == EMPTY || position.isNone()) return;

        if (piece >> 3) {
            state.blackMg -= tableMg(piece, position.index);
            state.blackEg -= tableEg(piece, position.index);
        } else {
            state.whiteMg -= tableMg(piece, position.index);
            state.whiteEg -= tableEg(piece, position.index);
        }

        state.evalPhase -= gamePhaseWeight(piece);
    }
    void refreshIncrementalEval() {
        clearIncrementalEval();

        for (Position position; position <= Position(7, 7); position++) {
            addPieceToEval(getPiece(position), position);
        }
    }
    void refreshPieceBits() {
        state.pieceBits.fill(0);

        for (Position position; position <= Position(7, 7); position++) {
            Piece piece = getPiece(position);
            if (piece != EMPTY)
                state.pieceBits[piece] |= 1ULL << position.index;
        }
    }

    void resetBoard() {

        state = BoardState();
        stackTop = 0;

        state.board[7] = 0xCABDEBAC;
        state.board[6] = 0x99999999;
        state.board[5] = 0;
        state.board[4] = 0;
        state.board[3] = 0;
        state.board[2] = 0;
        state.board[1] = 0x11111111;
        state.board[0] = 0x42356324;

        state.whiteKing = findKing(false);
        state.blackKing = findKing(true);
    }
    void movePiece(const Position starting, const Position target, const Piece piece) {
        clear(starting);

        set(target, piece);
    }
    void clear(Position position) {
        Piece oldPiece = getPiece(position);
        if (oldPiece != EMPTY)
            state.pieceBits[oldPiece] &= ~(1ULL << position.index);

        state.board[position.rank()] &= ~(0xF << (7-position.file())*4);
    }
    void set(Position position, Piece piece) {
        clear(position);
        if (piece != EMPTY)
            state.pieceBits[piece] |= 1ULL << position.index;

        state.board[position.rank()] |= piece << (7-position.file())*4;
    }

    void getPossibleMoves(const Position position, MoveList& moves) const {
        Piece piece = getPiece(position);

        const int rank = position.rank();
        const int file = position.file();

        if (piece == EMPTY) return;

        PieceType enemy = piece >> 3 ? WHITE : BLACK;

        switch (piece) {
            case WHITE_PAWN: case BLACK_PAWN: {

                int pawnDir = enemy == WHITE ? -1 : 1;

                // forward 1
                Position target1{rank + pawnDir, file};
                if (getPieceType(target1) == NONE) moves.push({position, target1, target1.rank() == 0 || target1.rank() == 7 ? PromotionQ : None});

                // forward 2
                Position target2{rank + 2*pawnDir, file};
                if (getPieceType(target1) == NONE && getPieceType(target2) == NONE && rank == (enemy == WHITE ? 6 : 1)) moves.push({position, target2});

                // attack right
                Position targetR{rank + pawnDir, file+1};
                if (!targetR.isNone() && getPieceType(targetR) == enemy) moves.push({position, targetR, targetR.rank() == 0 || targetR.rank() == 7 ? PromotionQC : Capture});

                // attack left
                Position targetL{rank + pawnDir, file-1};
                if (!targetL.isNone() && getPieceType(targetL) == enemy) moves.push({position, targetL, targetL.rank() == 0 || targetL.rank() == 7 ? PromotionQC : Capture});

                // en passant right
                Position targetER{rank, file+1};
                if (!targetER.isNone() && targetER == state.lastPawnMoved2) moves.push({position, targetR, EnPassant});

                // en passant left
                Position targetEL{rank, file-1};
                if (!targetEL.isNone() && targetEL == state.lastPawnMoved2) moves.push({position, targetL, EnPassant});

                break;
            }

            case WHITE_KNIGHT: case BLACK_KNIGHT: {
                for (auto& offset : knightOffsets) {
                    Position target = position;
                    if (!target.add(offset[0], offset[1])) continue;

                    PieceType targetPieceType = getPieceType(target);

                    if (targetPieceType == NONE) {
                        moves.push({position, target});
                    }
                    else if (targetPieceType == enemy) {
                        moves.push({position, target, Capture});
                    }
                }

                break;
            }

            case WHITE_BISHOP: case BLACK_BISHOP: {
                for (auto& dir : bishopDirections) {
                    Position target = position;
                    for (int i = 1; i < 8; i++) {
                        if (!target.add(dir[0], dir[1])) break;

                        PieceType targetPieceType = getPieceType(target);

                        if (targetPieceType == NONE) {
                            moves.push({position, target});
                        }
                        else {
                            if (targetPieceType == enemy)
                                moves.push({position, target, Capture});

                            break;
                        }
                    }
                }

                break;
            }

            case WHITE_ROOK: case BLACK_ROOK: {
                for (auto& dir : rookDirections) {
                    Position target = position;
                    for (int i = 1; i < 8; i++) {
                        if (!target.add(dir[0], dir[1])) break;

                        PieceType targetPieceType = getPieceType(target);

                        if (targetPieceType == NONE) {
                            moves.push({position, target});
                        }
                        else {
                            if (targetPieceType == enemy)
                                moves.push({position, target, Capture});

                            break;
                        }
                    }
                }

                break;
            }

            case WHITE_QUEEN: case BLACK_QUEEN: {
                for (auto& dir : queenDirections) {
                    Position target = position;
                    for (int i = 1; i < 8; i++) {
                        if (!target.add(dir[0], dir[1])) break;

                        PieceType targetPieceType = getPieceType(target);

                        if (targetPieceType == NONE) {
                            moves.push({position, target});
                        }
                        else {
                            if (targetPieceType == enemy)
                                moves.push({position, target, Capture});

                            break;
                        }
                    }
                }

                break;
            }

            case WHITE_KING: {
                for (auto& offset : kingOffsets) {
                    Position target = position;
                    if (!target.add(offset[0], offset[1])) continue;

                    PieceType targetPieceType = getPieceType(target);

                    if (targetPieceType == NONE) {
                        moves.push({position, target});
                    }
                    else if (targetPieceType == BLACK) {
                        moves.push({position, target, Capture});
                    }
                }

                if (state.whiteCastleKing) {
                    if (
                        getPiece({0, 4}) == WHITE_KING &&
                        getPiece({0, 7}) == WHITE_ROOK &&
                        getPiece({0, 5}) == EMPTY &&
                        getPiece({0, 6}) == EMPTY
                    ) {
                        moves.push({position, {0, 6}, CastleKing});
                    }
                }

                if (state.whiteCastleQueen) {
                    if (
                        getPiece({0, 4}) == WHITE_KING &&
                        getPiece({0, 0}) == WHITE_ROOK &&
                        getPiece({0, 3}) == EMPTY &&
                        getPiece({0, 2}) == EMPTY &&
                        getPiece({0, 1}) == EMPTY
                    ) {
                        moves.push({position, {0, 2}, CastleQueen});
                    }
                }

                break;
            }

            case BLACK_KING: {
                for (auto& offset : kingOffsets) {
                    Position target = position;
                    if (!target.add(offset[0], offset[1])) continue;

                    PieceType targetPieceType = getPieceType(target);

                    if (targetPieceType == NONE) {
                        moves.push({position, target});
                    }
                    else if (targetPieceType == WHITE) {
                        moves.push({position, target, Capture});
                    }
                }

                if (state.blackCastleKing) {
                    if (
                        getPiece({7, 4}) == BLACK_KING &&
                        getPiece({7, 7}) == BLACK_ROOK &&
                        getPiece({7, 5}) == EMPTY &&
                        getPiece({7, 6}) == EMPTY
                    ) {
                        moves.push({position, {7, 6}, CastleKing});
                    }
                }

                if (state.blackCastleQueen) {
                    if (
                        getPiece({7, 4}) == BLACK_KING &&
                        getPiece({7, 0}) == BLACK_ROOK &&
                        getPiece({7, 3}) == EMPTY &&
                        getPiece({7, 2}) == EMPTY &&
                        getPiece({7, 1}) == EMPTY
                    ) {
                        moves.push({position, {7, 2}, CastleQueen});
                    }
                }

                break;
            }

            default: ;
        }
    }
    void getPossibleCaptures(const Position position, MoveList& moves) const {
        Piece piece = getPiece(position);

        const int rank = position.rank();
        const int file = position.file();

        if (piece == EMPTY) return;

        PieceType enemy = piece >> 3 ? WHITE : BLACK;

        switch (piece) {
            case WHITE_PAWN: case BLACK_PAWN: {

                int pawnDir = enemy == WHITE ? -1 : 1;

                // attack right
                Position targetR{rank + pawnDir, file+1};
                if (!targetR.isNone() && getPieceType(targetR) == enemy) moves.push({position, targetR, targetR.rank() == 0 || targetR.rank() == 7 ? PromotionQC : Capture});

                // attack left
                Position targetL{rank + pawnDir, file-1};
                if (!targetL.isNone() && getPieceType(targetL) == enemy) moves.push({position, targetL, targetL.rank() == 0 || targetL.rank() == 7 ? PromotionQC : Capture});

                // en passant right
                Position targetER{rank, file+1};
                if (!targetER.isNone() && targetER == state.lastPawnMoved2) moves.push({position, targetR, EnPassant});

                // en passant left
                Position targetEL{rank, file-1};
                if (!targetEL.isNone() && targetEL == state.lastPawnMoved2) moves.push({position, targetL, EnPassant});

                break;
            }

            case WHITE_KNIGHT: case BLACK_KNIGHT: {
                for (auto& offset : knightOffsets) {
                    Position target = position;
                    if (!target.add(offset[0], offset[1])) continue;

                    PieceType targetPieceType = getPieceType(target);

                    if (targetPieceType == enemy) {
                        moves.push({position, target, Capture});
                    }
                }

                break;
            }

            case WHITE_BISHOP: case BLACK_BISHOP: {
                for (auto& dir : bishopDirections) {
                    Position target = position;
                    for (int i = 1; i < 8; i++) {
                        if (!target.add(dir[0], dir[1])) break;

                        PieceType targetPieceType = getPieceType(target);

                        if (targetPieceType != NONE) {
                            if (targetPieceType == enemy) {
                                moves.push({position, target, Capture});
                            }
                            break;
                        }
                    }
                }

                break;
            }

            case WHITE_ROOK: case BLACK_ROOK: {
                for (auto& dir : rookDirections) {
                    Position target = position;
                    for (int i = 1; i < 8; i++) {
                        if (!target.add(dir[0], dir[1])) break;

                        PieceType targetPieceType = getPieceType(target);

                        if (targetPieceType != NONE) {
                            if (targetPieceType == enemy) {
                                moves.push({position, target, Capture});
                            }
                            break;
                        }
                    }
                }

                break;
            }

            case WHITE_QUEEN: case BLACK_QUEEN: {
                for (auto& dir : queenDirections) {
                    Position target = position;
                    for (int i = 1; i < 8; i++) {
                        if (!target.add(dir[0], dir[1])) break;

                        PieceType targetPieceType = getPieceType(target);

                        if (targetPieceType != NONE) {
                            if (targetPieceType == enemy) {
                                moves.push({position, target, Capture});
                            }
                            break;
                        }
                    }
                }

                break;
            }

            case WHITE_KING: case BLACK_KING: {
                for (auto& offset : kingOffsets) {
                    Position target = position;
                    if (!target.add(offset[0], offset[1])) continue;

                    PieceType targetPieceType = getPieceType(target);

                    if (targetPieceType == enemy) {
                        moves.push({position, target, Capture});
                    }
                }

                break;
            }

            default: ;
        }
    }

    [[nodiscard]] bool squareAttacked(Position square, bool byBlack) const {
        int rank = square.rank();
        int file = square.file();

        // pawns
        if (byBlack) {
            if (getPiece({rank + 1, file - 1}) == BLACK_PAWN) return true;
            if (getPiece({rank + 1, file + 1}) == BLACK_PAWN) return true;
        } else {
            if (getPiece({rank - 1, file - 1}) == WHITE_PAWN) return true;
            if (getPiece({rank - 1, file + 1}) == WHITE_PAWN) return true;
        }

        // knights
        Piece knight = byBlack ? BLACK_KNIGHT : WHITE_KNIGHT;
        for (auto& o : knightOffsets) {
            if (getPiece({rank + o[0], file + o[1]}) == knight)
                return true;
        }

        // king
        Piece king = byBlack ? BLACK_KING : WHITE_KING;
        for (auto& o : kingOffsets) {
            if (getPiece({rank + o[0], file + o[1]}) == king)
                return true;
        }

        Piece bishop = byBlack ? BLACK_BISHOP : WHITE_BISHOP;
        Piece rook   = byBlack ? BLACK_ROOK   : WHITE_ROOK;
        Piece queen  = byBlack ? BLACK_QUEEN  : WHITE_QUEEN;

        // diagonals: bishop / queen
        for (auto& d : bishopDirections) {
            Position p = square;
            for (int i = 1; i < 8; i++) {
                if (!p.add(d[0], d[1])) break;

                Piece piece = getPiece(p);
                if (piece == EMPTY) continue;

                if (piece == bishop || piece == queen) return true;
                break;
            }
        }

        // orthogonals: rook / queen
        for (auto& d : rookDirections) {
            Position p = square;
            for (int i = 1; i < 8; i++) {
                if (!p.add(d[0], d[1])) break;

                Piece piece = getPiece(p);
                if (piece == EMPTY) continue;

                if (piece == rook || piece == queen) return true;
                break;
            }
        }

        return false;
    }

    [[nodiscard]] bool validMove(const Move& move) {
        bool player = getPiece(move.starting()) >> 3;

        if (move.isCastleKing()) {
            int rank = player ? 7 : 0;
            if (
                squareAttacked({rank, 4}, !player) ||
                squareAttacked({rank, 5}, !player) ||
                squareAttacked({rank, 6}, !player)
                )
                return false;
        }

        if (move.isCastleQueen()) {
            int rank = player ? 7 : 0;
            if (
                squareAttacked({rank, 4}, !player) ||
                squareAttacked({rank, 3}, !player) ||
                squareAttacked({rank, 2}, !player)
                )
                return false;
        }

        this->move(move);

        bool inCheck = check(player);

        this->undoMove();

        return !inCheck;
    }

    [[nodiscard]] bool noMoves(const bool player) {
        for (Position position; position <= Position(7, 7); position++) {
            Piece piece = getPiece(position);

            if (piece == EMPTY || piece >> 3 != player) continue;

            MoveList moves;
            getValidMoves(position, moves);
            if (moves.count > 0) return false;
        }

        return true;
    }
    [[nodiscard]] bool staleMate(const bool player) {
        return noMoves(player) && !check(player);
    }

    [[nodiscard]] Position findKing(bool player) const {
        Piece king = player ? BLACK_KING : WHITE_KING;

        for (Position p; p <= Position(7, 7); p++) {
            if (getPiece(p) == king)
                return p;
        }

        return Position{};
    }

    void switchPlayer() {
        state.playerTurn = !state.playerTurn;
    }
};

#endif //BOARD_H
