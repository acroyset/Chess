//
// Created by Andreas Royset on 5/20/26.
//

#ifndef BOARDSTATE_H
#define BOARDSTATE_H
#include <array>
#include <iostream>

enum PieceType : uint8_t {
	NONE,
	WHITE,
	BLACK
};

struct Position {

	uint8_t index;

	constexpr Position() : index(0) {}
	constexpr explicit Position(uint8_t i) : index(i) {}
	constexpr Position(int rank, int file) {
		 index = (rank >= 0 && rank < 8 && file >= 0 && file < 8) ?
			uint8_t(rank * 8 + file) :
			0x80;
	}

	bool operator == (const Position& other) const {
		return index == other.index;
	}
	bool operator != (const Position& other) const {
		return index != other.index;
	}
	bool operator < (const Position& other) const {
		return index < other.index;
	}
	bool operator > (const Position& other) const {
		return index > other.index;
	}
	bool operator <= (const Position& other) const {
		return index <= other.index;
	}
	bool operator >= (const Position& other) const {
		return index >= other.index;
	}
	void operator ++ (int) {
		index++;
	}

	void setNone() {
		index = 0x80;
	}

	[[nodiscard]] bool isNone() const {
		return index & 0x80;
	}

	[[nodiscard]] uint8_t rank() const {
		return index >> 3;
	}
	[[nodiscard]] uint8_t file() const {
		return index & 7;
	}

	[[nodiscard]] bool add(int dr, int df) {
		int r = int(rank()) + dr;
		int f = int(file()) + df;

		if (r < 0 || r >= 8 || f < 0 || f >= 8) {
			setNone();
			return false;
		}

		index = uint8_t(r * 8 + f);
		return true;
	}
};

enum MoveFlag : uint8_t {
	None        = 0b0000,
	Capture     = 0b1000,
	CastleKing  = 0b0010,
	CastleQueen = 0b0011,
	EnPassant   = 0b1001,
	PromotionQ  = 0b0100,
	PromotionR  = 0b0101,
	PromotionB  = 0b0110,
	PromotionN  = 0b0111,
	PromotionQC = 0b1100,
	PromotionRC = 0b1101,
	PromotionBC = 0b1110,
	PromotionNC = 0b1111,
};

class Move {
	uint16_t data; // FFFF ssssss tttttt

public:

	Move() {
		data = 0xFF;
	}
	Move(const Position starting, const Position target, MoveFlag flag = None) {
		if (starting.isNone() || target.isNone()) std::cerr << "Error making move" << std::endl;
		data = flag << 12 | starting.index << 6 | target.index;
	}

	bool operator==(const Move & other) const {
		return data == other.data;
	}

	[[nodiscard]] bool isNone() const {
		return data == 0xFF;
	}

	[[nodiscard]] bool isCapture() const {
		return data >> 15;
	}
	[[nodiscard]] bool isCastleKing() const {
		return data >> 12 == CastleKing;
	}
	[[nodiscard]] bool isCastleQueen() const {
		return data >> 12 == CastleQueen;
	}
	[[nodiscard]] bool isEnPassant() const {
		return data >> 12 == EnPassant;
	}
	[[nodiscard]] bool isPromotion() const {
		return data >> 14 & 1;
	}
	[[nodiscard]] uint8_t promotionPiece() const {
		if (!isPromotion()) std::cerr << "not promotion" << std::endl;
		return data >> 12 & 0b0011;
	}

	[[nodiscard]] Position starting() const {
		return Position(data >> 6 & 0x3F);
	}
	[[nodiscard]] Position target() const {
		return Position(data & 0x3F);
	}

	uint16_t getData() const{return data;}
};

struct MoveList {
	Move moves[256];
	int count = 0;

	void push(Move m) { moves[count++] = m; }

	Move& operator[](int idx) {return moves[idx];}
	Move& operator[](size_t idx) {return moves[idx];}

	void clear(){count = 0;}
};

enum Piece : uint8_t {

	EMPTY        = 0x0, // 0

	WHITE_PAWN   = 0x1, // 1
	WHITE_KNIGHT = 0x2, // 2
	WHITE_BISHOP = 0x3, // 3
	WHITE_ROOK   = 0x4, // 4
	WHITE_QUEEN  = 0x5, // 5
	WHITE_KING   = 0x6, // 6

	BLACK_PAWN   = 0x9, // 9
	BLACK_KNIGHT = 0xA, // 10
	BLACK_BISHOP = 0xB, // 11
	BLACK_ROOK   = 0xC, // 12
	BLACK_QUEEN  = 0xD, // 13
	BLACK_KING   = 0xE  // 14

};

struct BoardState {
	std::array<uint32_t, 8> board{};
	std::array<uint64_t, 16> pieceBits{};

	bool whiteCastleKing = true;
	bool whiteCastleQueen = true;
	bool blackCastleKing = true;
	bool blackCastleQueen = true;

	Position lastPawnMoved2{-1, -1};

	int lastPawnMoveOrCapture = 0;

	bool playerTurn = false; // false = white, true = black

	//cache
	Position whiteKing;
	Position blackKing;

	float whiteMg = 0.0f;
	float whiteEg = 0.0f;
	float blackMg = 0.0f;
	float blackEg = 0.0f;
	int evalPhase = 0;

	uint64_t hash{};

	bool operator == (const BoardState& other) const {
		return board == other.board &&
			whiteCastleKing  == other.whiteCastleKing &&
			whiteCastleQueen == other.whiteCastleQueen &&
			blackCastleKing  == other.blackCastleKing &&
			blackCastleQueen == other.blackCastleQueen &&
			lastPawnMoved2   == other.lastPawnMoved2 &&
			playerTurn       == other.playerTurn;
	}
};

#endif //BOARDSTATE_H
