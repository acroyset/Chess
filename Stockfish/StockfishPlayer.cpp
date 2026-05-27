//
// Created by Codex on 5/27/26.
//

#include "StockfishPlayer.h"

#include <array>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

struct UciProcess {
    pid_t pid = -1;
    int input = -1;
    int output = -1;

    void closePipes() {
        if (input != -1) {
            close(input);
            input = -1;
        }
        if (output != -1) {
            close(output);
            output = -1;
        }
    }

    void stop(bool force) {
        closePipes();
        if (pid <= 0) return;

        if (force) {
            kill(pid, SIGKILL);
        }

        int status = 0;
        waitpid(pid, &status, 0);
        pid = -1;
    }
};

std::optional<std::string> executablePath(const std::string& path) {
    if (path.empty()) return std::nullopt;

    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || std::filesystem::is_directory(path, ec)) {
        return std::nullopt;
    }

    if (access(path.c_str(), X_OK) == 0) {
        return path;
    }

    return std::nullopt;
}

std::optional<std::string> findStockfishPath() {
    if (const char* configured = std::getenv("CHESS_STOCKFISH_PATH")) {
        if (auto path = executablePath(configured)) return path;
    }

    const std::array<std::string, 4> directPaths = {
        "./stockfish",
        "/opt/homebrew/bin/stockfish",
        "/usr/local/bin/stockfish",
        "/usr/bin/stockfish"
    };

    for (const std::string& candidate : directPaths) {
        if (auto path = executablePath(candidate)) return path;
    }

    const char* pathEnv = std::getenv("PATH");
    if (pathEnv == nullptr) return std::nullopt;

    std::stringstream paths(pathEnv);
    std::string dir;
    while (std::getline(paths, dir, ':')) {
        if (dir.empty()) continue;
        std::filesystem::path candidate = std::filesystem::path(dir) / "stockfish";
        if (auto path = executablePath(candidate.string())) return path;
    }

    return std::nullopt;
}

std::string boardToUciFEN(const Board& board) {
    std::string fen;

    for (int rank = 7; rank >= 0; rank--) {
        int empty = 0;
        for (int file = 0; file < 8; file++) {
            Piece piece = board.getPiece({rank, file});
            if (piece == EMPTY) {
                empty++;
                continue;
            }

            if (empty > 0) {
                fen += char('0' + empty);
                empty = 0;
            }

            const char pieceChars[] = " PNBRQKxxpnbrqk";
            fen += pieceChars[int(piece)];
        }

        if (empty > 0) fen += char('0' + empty);
        if (rank > 0) fen += '/';
    }

    fen += board.getPlayerTurn() ? " b " : " w ";

    std::string castle;
    if (board.canCastleKingSide(false))  castle += 'K';
    if (board.canCastleQueenSide(false)) castle += 'Q';
    if (board.canCastleKingSide(true))   castle += 'k';
    if (board.canCastleQueenSide(true))  castle += 'q';
    fen += castle.empty() ? "-" : castle;
    fen += ' ';

    Position lastPawnMoved2 = board.getLastPawnMoved2();
    if (lastPawnMoved2.isNone()) {
        fen += '-';
    } else {
        int epRank = board.getPlayerTurn()
            ? int(lastPawnMoved2.rank()) - 1
            : int(lastPawnMoved2.rank()) + 1;
        fen += char('a' + lastPawnMoved2.file());
        fen += char('1' + epRank);
    }

    fen += ' ';
    fen += std::to_string(board.getHalfmoveClock());
    fen += " 1";
    return fen;
}

bool writeAll(int fd, const std::string& text) {
    const char* data = text.c_str();
    size_t remaining = text.size();

    while (remaining > 0) {
        ssize_t written = write(fd, data, remaining);
        if (written < 0) {
            if (errno == EINTR) continue;
            return false;
        }

        data += written;
        remaining -= size_t(written);
    }

    return true;
}

bool readLineWithDeadline(int fd, std::string& line, std::chrono::steady_clock::time_point deadline) {
    line.clear();

    while (std::chrono::steady_clock::now() < deadline) {
        auto now = std::chrono::steady_clock::now();
        auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
        timeval timeout{};
        timeout.tv_sec = long(remaining.count() / 1000);
        timeout.tv_usec = long((remaining.count() % 1000) * 1000);

        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(fd, &readSet);

        int ready = select(fd + 1, &readSet, nullptr, nullptr, &timeout);
        if (ready < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (ready == 0) return false;

        char c = '\0';
        ssize_t count = read(fd, &c, 1);
        if (count < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (count == 0) return !line.empty();
        if (c == '\r') continue;
        if (c == '\n') return true;

        line += c;
    }

    return false;
}

std::optional<UciProcess> launchEngine(const std::string& path, std::string& error) {
    int toChild[2]{};
    int fromChild[2]{};

    if (pipe(toChild) != 0 || pipe(fromChild) != 0) {
        error = "Could not create Stockfish pipes";
        return std::nullopt;
    }

    pid_t pid = fork();
    if (pid < 0) {
        error = "Could not start Stockfish";
        close(toChild[0]);
        close(toChild[1]);
        close(fromChild[0]);
        close(fromChild[1]);
        return std::nullopt;
    }

    if (pid == 0) {
        dup2(toChild[0], STDIN_FILENO);
        dup2(fromChild[1], STDOUT_FILENO);
        dup2(fromChild[1], STDERR_FILENO);

        close(toChild[0]);
        close(toChild[1]);
        close(fromChild[0]);
        close(fromChild[1]);

        execl(path.c_str(), path.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }

    close(toChild[0]);
    close(fromChild[1]);
    std::signal(SIGPIPE, SIG_IGN);

    UciProcess process;
    process.pid = pid;
    process.input = toChild[1];
    process.output = fromChild[0];

    return process;
}

MoveFlag promotionFlagFromUci(char promotion, bool capture) {
    switch (promotion) {
        case 'q': return capture ? PromotionQC : PromotionQ;
        case 'r': return capture ? PromotionRC : PromotionR;
        case 'b': return capture ? PromotionBC : PromotionB;
        case 'n': return capture ? PromotionNC : PromotionN;
        default: return capture ? Capture : None;
    }
}

std::optional<Move> resolveUciMove(Board& board, const std::string& uci) {
    if (uci.size() < 4) return std::nullopt;

    Position start{uci[1] - '1', uci[0] - 'a'};
    Position target{uci[3] - '1', uci[2] - 'a'};
    if (start.isNone() || target.isNone()) return std::nullopt;

    char promotion = uci.size() >= 5 ? char(std::tolower(uci[4])) : '\0';

    MoveList legalMoves;
    board.getValidMoves(legalMoves);
    for (int i = 0; i < legalMoves.count; i++) {
        Move move = legalMoves[i];
        if (move.starting() != start || move.target() != target) continue;
        if (promotion == '\0') return move;
        if (!move.isPromotion()) continue;

        if ((move.getData() >> 12) == promotionFlagFromUci(promotion, move.isCapture())) {
            return move;
        }
    }

    return std::nullopt;
}

void parseInfoLine(const std::string& line, bool blackToMove, int& depth, float& eval, bool& mate) {
    std::stringstream ss(line);
    std::string token;

    ss >> token;
    if (token != "info") return;

    while (ss >> token) {
        if (token == "depth") {
            ss >> depth;
        }
        else if (token == "score") {
            std::string scoreType;
            int value = 0;
            ss >> scoreType >> value;

            mate = scoreType == "mate";
            float sideToMoveEval = mate ? float(value) : float(value) / 100.0f;
            eval = blackToMove ? -sideToMoveEval : sideToMoveEval;
        }
    }
}

std::string buildPositionCommand(const Board& board, const std::string& rootFen, const std::vector<Move>& playedMoves) {
    std::string command;

    if (rootFen.empty()) {
        command = "position fen " + boardToUciFEN(board);
    } else {
        command = "position fen " + rootFen;
    }

    if (!playedMoves.empty()) {
        command += " moves";
        for (Move move : playedMoves) {
            command += ' ';
            command += moveToUCI(move);
        }
    }

    command += '\n';
    return command;
}

StockfishSearchResult runStockfish(
    Board board,
    const std::string& path,
    int moveTimeMs,
    const std::string& rootFen,
    const std::vector<Move>& playedMoves
) {
    StockfishSearchResult result;
    result.available = true;
    result.enginePath = path;
    result.boardHash = board.getHash();
    result.blackToMove = board.getPlayerTurn();

    std::string error;
    std::optional<UciProcess> maybeProcess = launchEngine(path, error);
    if (!maybeProcess.has_value()) {
        result.error = error;
        result.finished = true;
        return result;
    }

    UciProcess process = maybeProcess.value();
    bool forceKill = true;

    std::string commands;
    commands += "uci\n";
    commands += "isready\n";
    commands += "ucinewgame\n";
    commands += buildPositionCommand(board, rootFen, playedMoves);
    commands += "go movetime " + std::to_string(moveTimeMs) + "\n";

    if (!writeAll(process.input, commands)) {
        result.error = "Could not write to Stockfish";
        result.finished = true;
        process.stop(true);
        return result;
    }

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(moveTimeMs + 5000);
    std::string line;

    while (readLineWithDeadline(process.output, line, deadline)) {
        if (line.rfind("info ", 0) == 0) {
            parseInfoLine(line, result.blackToMove, result.depth, result.eval, result.mate);
        }
        else if (line.rfind("bestmove ", 0) == 0) {
            std::stringstream ss(line);
            std::string token;
            ss >> token >> result.bestMoveUci;
            result.bestMove = resolveUciMove(board, result.bestMoveUci);
            if (!result.bestMove.has_value()) {
                result.error = "Stockfish move was not legal on app board";
            }
            forceKill = false;
            break;
        }
    }

    if (result.bestMoveUci.empty()) {
        result.timedOut = true;
        result.error = "Stockfish timed out";
    } else {
        writeAll(process.input, "quit\n");
    }

    result.finished = true;
    process.stop(forceKill);
    return result;
}

} // namespace

std::string moveToUCI(Move move) {
    if (move.isNone()) return "--";

    std::string uci;
    uci += char('a' + move.starting().file());
    uci += char('1' + move.starting().rank());
    uci += char('a' + move.target().file());
    uci += char('1' + move.target().rank());

    if (move.isPromotion()) {
        switch (move.promotionPiece()) {
            case 0: uci += 'q'; break;
            case 1: uci += 'r'; break;
            case 2: uci += 'b'; break;
            case 3: uci += 'n'; break;
            default: break;
        }
    }

    return uci;
}

StockfishPlayer::~StockfishPlayer() {
    resetInput();
}

std::optional<Move> StockfishPlayer::selectMove(Board& board, sf::RenderWindow&) {
    if (!started) {
        resetInput();

        auto path = findStockfishPath();
        {
            std::lock_guard lock(resultMutex);
            currentResult = StockfishSearchResult{};
            currentResult.boardHash = board.getHash();
            currentResult.blackToMove = board.getPlayerTurn();
            if (!path.has_value()) {
                currentResult.finished = true;
                currentResult.error = "Stockfish not found";
                lastResult = currentResult;
                return std::nullopt;
            }
        }

        started = true;
        searching = true;
        Board boardCopy(board);
        std::string enginePath = path.value();
        int timeMs = moveTimeMs;
        std::string startFenCopy;
        std::vector<Move> playedMovesCopy;
        {
            std::lock_guard lock(resultMutex);
            startFenCopy = rootFen;
            playedMovesCopy = playedMoves;
        }

        thread = std::thread([this, boardCopy, enginePath, timeMs, startFenCopy, playedMovesCopy]() mutable {
            StockfishSearchResult result = runStockfish(boardCopy, enginePath, timeMs, startFenCopy, playedMovesCopy);
            {
                std::lock_guard lock(resultMutex);
                currentResult = result;
                lastResult = result;
            }
            searching = false;
        });

        return std::nullopt;
    }

    if (searching.load()) return std::nullopt;

    if (thread.joinable()) {
        thread.join();
    }

    started = false;
    std::lock_guard lock(resultMutex);
    return currentResult.bestMove;
}

void StockfishPlayer::resetInput() {
    if (thread.joinable()) {
        thread.join();
    }
    searching = false;
    started = false;
}

void StockfishPlayer::setGameHistory(const std::string& startFen, const std::vector<Move>& moves) {
    if (searching.load()) return;

    std::lock_guard lock(resultMutex);
    rootFen = startFen;
    playedMoves = moves;
}

StockfishSearchResult StockfishPlayer::getLastSearch() const {
    std::lock_guard lock(resultMutex);
    return lastResult;
}

bool StockfishPlayer::isAvailable() {
    return findStockfishPath().has_value();
}
