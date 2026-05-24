//
// Created by Andreas Royset on 5/21/26.
//

#ifndef BITBOARD_H
#define BITBOARD_H
#include "BoardState.h"

class BitBoard {
	uint64_t data;

public:

	BitBoard() {
		data = 0;
	}
	explicit BitBoard(const uint64_t data) {
		this->data = data;
	}
	BitBoard(const BitBoard& other) {
		data = other.data;
	}

	BitBoard operator | (const BitBoard& other) const {
		return BitBoard(data | other.data);
	}
	void operator |= (const BitBoard& other) {
		data |= other.data;
	}

	BitBoard operator & (const BitBoard& other) const {
		return BitBoard(data & other.data);
	}
	void operator &= (const BitBoard& other) {
		data &= other.data;
	}


	[[nodiscard]] bool get(const Position& position) const {
		int index = position.rank()*8 + position.file();
		return data >> index & 1ULL;
	}
	void set(const Position& position, const bool value) {
		const int index = position.rank() * 8 + position.file();
		const uint64_t mask = 1ULL << index;

		if (value)
			data |= mask;
		else
			data &= ~mask;
	}

	void clear() {
		data = 0;
	}
};

#endif //BITBOARD_H
