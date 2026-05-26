//
// Created by Andreas Royset on 5/25/26.
//

#ifndef FENSAVELOADER_H
#define FENSAVELOADER_H

// ---- FEN export ----
inline std::string boardToFEN(const Board& board) {
    std::string fen;
    for (int rank = 7; rank >= 0; rank--) {
        int empty = 0;
        for (int file = 0; file < 8; file++) {
            Piece p = board.getPiece({rank, file});
            if (p == EMPTY) {
                empty++;
            } else {
                if (empty > 0) { fen += char('0' + empty); empty = 0; }
                const char pieceChars[] = " PNBRQKxpnbrqk";
                fen += pieceChars[int(p)];
            }
        }
        if (empty > 0) fen += char('0' + empty);
        if (rank > 0) fen += '/';
    }
    fen += ' ';
    fen += board.getPlayerTurn() ? 'b' : 'w';
    fen += ' ';
    std::string castle;
    if (board.canCastleKingSide(false))  castle += 'K';
    if (board.canCastleQueenSide(false)) castle += 'Q';
    if (board.canCastleKingSide(true))   castle += 'k';
    if (board.canCastleQueenSide(true))  castle += 'q';
    if (castle.empty()) castle = "-";
    fen += castle;
    fen += " - 0 1";
    return fen;
}

// ---- PGN export ----
inline std::string exportPGN(const std::vector<MoveRecord>& history) {
    std::string pgn;
    pgn += "[Event \"Local Game\"]\n[Site \"Chess App\"]\n";
    pgn += "[Date \"????.??.??\"]\n[White \"?\"]\n[Black \"?\"]\n[Result \"*\"]\n\n";
    for (int i = 0; i < int(history.size()); i++) {
        if (i % 2 == 0) pgn += std::to_string(i / 2 + 1) + ". ";
        pgn += history[i].san + " ";
    }
    pgn += "*\n";
    return pgn;
}

// ---- macOS clipboard helpers ----
inline void copyToClipboard(const std::string& text) {
    FILE* pipe = popen("pbcopy", "w");
    if (pipe) { fwrite(text.c_str(), 1, text.size(), pipe); pclose(pipe); }
}

inline std::string pasteFromClipboard() {
    std::string result;
    FILE* pipe = popen("pbpaste", "r");
    if (pipe) {
        char buf[4096];
        while (fgets(buf, sizeof(buf), pipe)) result += buf;
        pclose(pipe);
    }
    while (!result.empty() && (result.back()=='\n'||result.back()==' '||result.back()=='\r'))
        result.pop_back();
    return result;
}

// ---- FEN captured-piece inference ----
// Call this after loading a FEN. Starting counts per piece type (index = Piece enum value).
inline std::array<int, 16> inferCapturedFromFEN(const Board& board) {
    // Starting piece counts indexed by Piece enum
    constexpr int startCounts[16] = {
        0,  // EMPTY
        8,  // WHITE_PAWN
        2,  // WHITE_KNIGHT
        2,  // WHITE_BISHOP
        2,  // WHITE_ROOK
        1,  // WHITE_QUEEN
        1,  // WHITE_KING
        0, 0, // unused (7, 8)
        8,  // BLACK_PAWN
        2,  // BLACK_KNIGHT
        2,  // BLACK_BISHOP
        2,  // BLACK_ROOK
        1,  // BLACK_QUEEN
        1,  // BLACK_KING
        0
    };
    int boardCounts[16] = {};
    for (Position pos; pos <= Position(7,7); pos++) {
        Piece p = board.getPiece(pos);
        if (p != EMPTY && int(p) < 16) boardCounts[int(p)]++;
    }
    std::array<int, 16> captured{};
    for (int p = 1; p <= 14; p++) {
        if (p == 7 || p == 8) continue;
        captured[p] = std::max(0, startCounts[p] - boardCounts[p]);
    }
    return captured;
}

#endif //FENSAVELOADER_H
