//
// Created by Andreas Royset on 5/22/26.
//

#ifndef TRANSPOSITIONTABLE_H
#define TRANSPOSITIONTABLE_H

#include <cstdint>
#include "../Game/BoardState.h"

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

// -----------------------------------------------------------------------
// Two-bucket transposition table.
//
// Each slot holds two entries:
//   bucket[0] — depth-preferred:  only replaced if new entry is deeper
//   bucket[1] — always-replace:   always replaced (catches recent positions)
//
// On lookup we check both and return the one with matching hash.
// This dramatically increases effective table utilisation compared to a
// single depth-preferred replacement scheme.
// -----------------------------------------------------------------------
struct TTSlot {
    Entry depth;   // depth-preferred bucket
    Entry always;  // always-replace bucket
};

class TranspositionTable {
	std::vector<TTSlot> slots;
	size_t mask;
	size_t size;

public:

	TranspositionTable() {
		// 128 MB total — each slot holds 2 entries
		size_t bytes  = 128ull * 1024ull * 1024ull;
		size_t count  = bytes / sizeof(TTSlot);

		size_t power = 1;
		while (power * 2 <= count)
			power *= 2;

		size = power;
		slots.resize(power);
		mask = power - 1;
	}

	[[nodiscard]] Entry find(uint64_t hash) const {
		const TTSlot& slot = slots[hash & mask];

		// Depth-preferred bucket takes priority on a match
		if (slot.depth.hash == hash && slot.depth.flag != FAIL)
			return slot.depth;

		if (slot.always.hash == hash && slot.always.flag != FAIL)
			return slot.always;

		// Return empty entry (flag == FAIL signals miss)
		return Entry{};
	}

	void store(const Entry& entry) {
		TTSlot& slot = slots[entry.hash & mask];

		// Always-replace: just overwrite
		slot.always = entry;

		// Depth-preferred: replace if slot is empty, hash matches (update),
		// or new entry is at least as deep
		if (slot.depth.flag == FAIL ||
		    slot.depth.hash == entry.hash ||
		    entry.depth >= slot.depth.depth)
		{
			slot.depth = entry;
		}
	}

	void clear() {
		slots.clear();
		slots.resize(size);
	}
};

#endif //TRANSPOSITIONTABLE_H