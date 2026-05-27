//
// Created by Andreas Royset on 5/26/26.
//

#ifndef OFFSETS_H
#define OFFSETS_H

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

#endif //OFFSETS_H
