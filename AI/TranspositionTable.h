//
// Created by Andreas Royset on 5/22/26.
//

#ifndef TRANSPOSITIONTABLE_H
#define TRANSPOSITIONTABLE_H

#include <cstdint>
#include "../BoardState.h"

enum Flag : uint8_t {
	FAIL,
	EXACT,
	LOWERBOUND,
	UPPERBOUND
};

struct Entry {
	uint64_t hash{};

	float eval{};
	int8_t depth{};

	Move bestMove;

	Flag flag;

	Entry() {
		flag = FAIL;
	}
};

class TranspositionTable {
	std::vector<Entry> entries;

	size_t mask;
	size_t size;

public:

	TranspositionTable() {
		size_t bytes = 128ull * 1024ull * 1024ull;
		size_t count = bytes / sizeof(Entry);

		size_t power = 1;
		while (power * 2 <= count)
			power *= 2;

		size = power;
		entries.resize(power);
		mask = power - 1;
	}

	[[nodiscard]] const Entry& find(const uint64_t hash) const {
		const uint64_t index = hash & mask;

		const Entry& entry = entries[index];
		if (entry.hash == hash) {
			return entry;
		}

		static Entry empty{};
		return empty;
	}

	void store(const Entry& entry) {
		const uint64_t index = entry.hash & mask;

		Entry& old = entries[index];

		if (old.flag == FAIL || entry.depth >= old.depth) {
			old = entry;
		}
	}

	void clear() {
		entries.clear();
		entries.resize(size);
	}
};

#endif //TRANSPOSITIONTABLE_H
