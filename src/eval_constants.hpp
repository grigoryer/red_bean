#pragma once

#include "global.hpp"
#include "search_globals.hpp"

constexpr std::array<Score, kSquareCount> kQueenPosEval = 
{
0, 0, 5, 5, 5, 5, 0, 0,
0, 5, 10, 10, 10, 10, 5, 0,
5, 10, 15, 15, 15, 15, 10, 5,
5, 10, 15, 20, 20, 15, 10, 5,
5, 10, 15, 20, 20, 15, 10, 5,
5, 10, 15, 15, 15, 15, 10, 5,
0, 5, 10, 10, 10, 10, 5, 0,
0, 0, 5, 5, 5, 5, 0, 0
};

constexpr std::array<Score, kSquareCount> kKnightPosEval = 
{
-20, -10, -5, -5, -5, -5, -10, -20,
-10, 0, 10, 10, 10, 10, 0, -10,
-5, 10, 20, 20, 20, 20, 10, -5,
-5, 10, 20, 25, 25, 20, 10, -5,
-5, 10, 20, 25, 25, 20, 10, -5,
-5, 10, 20, 20, 20, 20, 10, -5,
-10, 0, 10, 10, 10, 10, 0, -10,
-20, -10, -5, -5, -5, -5, -10, -20
};

constexpr std::array<Score, kSquareCount> kBishopPosEval = 
{
-10, 0, 0, 10, 10, 0, 0, -10,
0, 10, 10, 20, 20, 10, 10, 0,
0, 10, 20, 20, 20, 20, 10, 0,
10, 20, 20, 25, 25, 20, 20, 10,
10, 20, 20, 25, 25, 20, 20, 10,
0, 10, 20, 20, 20, 20, 10, 0,
0, 10, 10, 20, 20, 10, 10, 0,
-10, 0, 0, 10, 10, 0, 0, -10
};

constexpr std::array<Score, kSquareCount> kRookPosEval = 
{
0, 0, 0, 20, 20, 0, 0, 0,
15, 15, 15, 25, 25, 15, 15, 15,
0, 0, 15, 25, 25, 15, 0, 0,
0, 0, 25, 35, 35, 25, 0, 0,
0, 0, 25, 35, 35, 25, 0, 0,
0, 0, 15, 25, 25, 15, 0, 0,
15, 25, 40, 40, 40, 40, 25, 15,
0, 0, 15, 15, 15, 15, 0, 0
};


constexpr std::array<Score, kSquareCount> kPawnPosEval = 
{
0, 0, 0, 0, 0, 0, 0, 0,
5, 5, 5, 5, 5, 5, 5, 5,
10, 10, 30, 40, 40, 30, 10, 10,
15, 15, 30, 40, 40, 30, 15, 15,
20, 20, 20, 40, 40, 20, 20, 20,
25, 25, 25, 25, 25, 25, 25, 25,
30, 30, 30, 30, 30, 30, 30, 30,
0, 0, 0, 0, 0, 0, 0, 0 // rank 8 doesnt matter since it is impossible
};

constexpr std::array<Score, kSquareCount> kPawnLatePosEval = 
{
0, 0, 0, 0, 0, 0, 0, 0,
-5, -5, -5, -5, -5, -5, -5, -5,
-10, -10, -10, -10, -10, -10, -10, -10,
30, 30, 30, 40, 40, 30, 30, 30,
40, 40, 40, 40, 40, 40, 40, 40,
50, 50, 50, 50, 50, 50, 50, 50,
60, 60, 60, 60, 60, 60, 60, 60,
0, 0, 0, 0, 0, 0, 0, 0 // rank 8 doesnt matter since it is impossible
};


constexpr std::array<Score, 64> kKingPosEval = 
{
10, 35, 10,  0,  0, 10, 35, 10,
0, -5, -5, -5, -5, -5, -5, 0,
0,  0, -20, -30, -30, -20,  0,  0,
-20, -30, -40, -50, -50, -40, -30, -20,
-30, -40, -50, -60, -60, -50, -40, -30,
-40, -50, -60, -70, -70, -60, -50, -40,
-50, -60, -70, -80, -80, -70, -60, -50,
-60, -70, -80, -90, -90, -80, -70, -60
};

constexpr std::array<Score, 64> kKingLatePosEval =
{
-15, -10, -5, -5, -5, -5, -10, -15,
-10, -5, 0, 0, 0, 0, -5, -10,
-5, 0, 10, 10, 10, 10, 0, -5,
-5, 0, 20, 20, 20, 20, 0, -5,
-5, 0, 20, 20, 20, 20, 0, -5,
-5, 0, 10, 10, 10, 10, 0, -5,
-10, -5, 0, 0, 0, 0, -5, -10,
-15, -10, -5, -5, -5, -5, -10, -15
};

constexpr Score kPawnNeighborBonus = 4;
constexpr Score kPassedPawnBonus = 20;
constexpr Score kPawnShieldBonus = 25;

constexpr std::array<Score, kPieceCount> kVisionBonuses = 
{
    1, // queen
    2, // rook
    4, // bishop
    4, // knight
    0, // pawn
    0  // king
};

constexpr std::array<Score, kPieceCount> kPieceValues = 
{
    900, // queen
    500, // rook
    300, // bishop
    300, // knight
    100, // pawn
    0    // king
};