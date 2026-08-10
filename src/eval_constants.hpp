#pragma once

#include "global.hpp"
#include "search_globals.hpp"
#include <cstdint>

namespace game_eval
{
enum class Phase : uint8_t { kMg, kEg };

constexpr Score kTempoBonus = 10;

constexpr Score kBishopPairBonus = 30;
constexpr Score kKnightPairPenalty = -8;
constexpr Score kRookPairPenalty = -20;
constexpr Score kTradeDownBonus = 10;
constexpr Score kNoPawnsPenalty = -30;

constexpr int kStartNonPawnMaterial = 6200;

// King Safety

// Per pawn on x rows in front it. So [0] = current row and is skipped, [1] = one row in front
constexpr int kPawnShieldRowsInfront{3}; 
constexpr std::array<Score, kPawnShieldRowsInfront> kPawnShieldBonus = {0, 5, 2};

constexpr int kPawnStormRowsInfront{5};
constexpr std::array<Score, kPawnStormRowsInfront> kPawnStormPenalty = {0, -2, -2, -1, -1};

constexpr Score kPawnShieldPenality = -7;


// Pawn structure

// Passed pawn
constexpr std::array<std::array<Score, kRowCount>, 2> kPassedPawnBonus =
{{
    // Middlegame
    { 0, 2, 3, 4, 5, 6, 7, 0 },
    // Endgame
    { 0, 3, 6, 8, 10, 12, 14, 0 },
}};

constexpr std::array<std::array<Score, kRowCount>, 2> kCandidatePassedPawnBonus =
{{
    // Middlegame
    { 0, 1, 1, 2, 2, 3, 3, 0 },
    // Endgame
    { 0, 2, 3, 4, 5, 6, 7, 0 },
}};

// Isolated pawn: no friendly pawns on adjacent files
constexpr std::array<Score, 2> kIsolatedPawnPenalty = { -5, -7 };  // { MG, EG }

// Doubled pawn: applied per extra pawn beyond the first on a file. Worse in EG (fewer pieces to compensate).
constexpr std::array<Score, 2> kDoubledPawnPenalty = { -4, -8 };  // { MG, EG }

// Backward pawn: stop square lacks pawn protection and is controlled by a sentry.
constexpr std::array<Score, 2> kBackwardPawnPenalty = { -3, -2 };  // { MG, EG }

// For how many islands, maximum there can only be 4 islands
constexpr std::array<std::array<Score, 9>, 2> kPawnIslandsPenalty =
{{
    // Middlegame
    { 0, 0, -1, -2, -3, 0, 0, 0, 0 },
    // Endgame
    { 0, 0, -2, -4, -6, 0, 0, 0, 0 },
}};

constexpr std::array<std::array<Score, 5>, 2> kPawnMajorityBonus =
{{
    // Middlegame
    { 0, 1, 2, 3, 4 },
    // Endgame
    { 0, 3, 5, 7, 9 },
}};

constexpr Bitboard kKingsideMask = kColumnsBitboards[static_cast<size_t>(Column::F)] | 
                                   kColumnsBitboards[static_cast<size_t>(Column::G)] | 
                                   kColumnsBitboards[static_cast<size_t>(Column::H)];


constexpr Bitboard kQueensideMask = kColumnsBitboards[static_cast<size_t>(Column::A)] | 
                                    kColumnsBitboards[static_cast<size_t>(Column::B)] | 
                                    kColumnsBitboards[static_cast<size_t>(Column::C)] | 
                                    kColumnsBitboards[static_cast<size_t>(Column::D)];


// Indexed by the rank of the defending pawn.
constexpr std::array<std::array<Score, kRowCount>, 2> kChainPawnBonus =
{{
    // Middlegame
    { 0, 1, 2, 3, 3, 4, 0, 0 },
    // Endgame
    { 0, 2, 4, 6, 6, 8, 0, 0 },
}};

// Phalanx: indexed by the shared rank. Counted once per pawn, so a side-by-side pair
// scores this twice. Zero on rank 2 so the untouched starting wall earns nothing.
constexpr std::array<std::array<Score, kRowCount>, 2> kPhalanxPawnBonus =
{{
    // Middlegame
    { 0, 0, 1, 1, 2, 2, 2, 0 },
    // Endgame
    { 0, 0, 2, 2, 3, 3, 3, 0 },
}};

constexpr std::array<Score, kPieceCount> kMG_VisionBonuses = 
{
    1, // queen
    2, // rook
    4, // bishop
    4, // knight
    0, // pawn
    -1  // king
};

constexpr std::array<Score, kPieceCount> kEG_VisionBonuses = 
{
    2, // queen
    4, // rook
    5, // bishop
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

constexpr std::array<Score, kSquareCount> kMG_QueenPST = 
{
    -10,  -5,  -5,   0,   0,  -5,  -5, -10,
     -5,   0,   5,   5,   5,   5,   0,  -5,
     -5,   5,   5,   5,   5,   5,   5,  -5,
      0,   5,   5,   8,   8,   5,   5,  -5,
     -5,   5,   5,   8,   8,   5,   5,  -5,
     -5,   5,   5,   5,   5,   5,   5,  -5,
     -5,   0,   5,   5,   5,   5,   0,  -5,
    -10,  -5,  -5,   0,   0,  -5,  -5, -10
};

constexpr std::array<Score, kSquareCount> kEG_QueenPST = 
{
    -20, -15, -10,  -5,  -5, -10, -15, -20,
    -15,  -5,   0,   0,   0,   0,  -5, -15,
    -10,   0,   5,  10,  10,   5,   0, -10,
     -5,   0,  10,  15,  15,  10,   0,  -5,
     -5,   0,  10,  15,  15,  10,   0,  -5,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -15,  -5,   0,   0,   0,   0,  -5, -15,
    -20, -15, -10,  -5,  -5, -10, -15, -20
};

constexpr std::array<Score, kSquareCount> kMG_RookPST = 
{
     -5,   0,   5,  10,  10,   5,   0,  -5,
     -5,   0,   0,   5,   5,   0,   0,  -5,
     -5,   0,   0,   5,   5,   0,   0,  -5,
     -5,   0,   0,   5,   5,   0,   0,  -5,
     -5,   0,   0,   5,   5,   0,   0,  -5,
     -5,   0,   0,   5,   5,   0,   0,  -5,
     15,  20,  20,  20,  20,  20,  20,  15,
      0,   0,   0,   5,   5,   0,   0,   0
};

constexpr std::array<Score, kSquareCount> kEG_RookPST = 
{
     -5,  -3,   0,   0,   0,   0,  -3,  -5,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      3,   3,   3,   3,   3,   3,   3,   3,
      5,   5,   5,   5,   5,   5,   5,   5,
      8,   8,   8,   8,   8,   8,   8,   8,
     12,  12,  12,  12,  12,  12,  12,  12,
      5,   5,   5,   5,   5,   5,   5,   5
};

constexpr std::array<Score, kSquareCount> kMG_BishopPST = 
{
    -15,  -5, -10,  -5,  -5, -10,  -5, -15,
     -5,  10,   5,   5,   5,   5,  10,  -5,
     -5,   8,  10,  10,  10,  10,   8,  -5,
     -5,   5,  10,  15,  15,  10,   5,  -5,
     -5,   5,  10,  15,  15,  10,   5,  -5,
     -5,   8,  10,  10,  10,  10,   8,  -5,
     -5,  10,   5,   5,   5,   5,  10,  -5,
    -15,  -5, -10,  -5,  -5, -10,  -5, -15
};

constexpr std::array<Score, kSquareCount> kEG_BishopPST = 
{
    -15, -10,  -8,  -5,  -5,  -8, -10, -15,
    -10,   0,   0,   3,   3,   0,   0, -10,
     -8,   0,   5,   8,   8,   5,   0,  -8,
     -5,   3,   8,  10,  10,   8,   3,  -5,
     -5,   3,   8,  10,  10,   8,   3,  -5,
     -8,   0,   5,   8,   8,   5,   0,  -8,
    -10,   0,   0,   3,   3,   0,   0, -10,
    -15, -10,  -8,  -5,  -5,  -8, -10, -15
};

constexpr std::array<Score, kSquareCount> kMG_KnightPST = 
{
    -50, -30, -15, -10, -10, -15, -30, -50,
    -30, -10,   5,  10,  10,   5, -10, -30,
    -15,   5,  15,  18,  18,  15,   5, -15,
    -10,  10,  18,  22,  22,  18,  10, -10,
    -10,  10,  18,  22,  22,  18,  10, -10,
    -15,   5,  15,  18,  18,  15,   5, -15,
    -30, -10,   5,  10,  10,   5, -10, -30,
    -50, -30, -15, -10, -10, -15, -30, -50
};

constexpr std::array<Score, kSquareCount> kEG_KnightPST = 
{
    -40, -25, -15, -10, -10, -15, -25, -40,
    -25, -10,   0,   5,   5,   0, -10, -25,
    -15,   0,   8,  12,  12,   8,   0, -15,
    -10,   5,  12,  15,  15,  12,   5, -10,
    -10,   5,  12,  15,  15,  12,   5, -10,
    -15,   0,   8,  12,  12,   8,   0, -15,
    -25, -10,   0,   5,   5,   0, -10, -25,
    -40, -25, -15, -10, -10, -15, -25, -40
};


constexpr std::array<Score, kSquareCount> kMG_PawnPST = 
{
      0,   0,   0,   0,   0,   0,   0,   0,
     20,  30,  15,  20,  15,  30,  25,  -5,
      0,   0,   7,   5,  10,  10,  10, -10,
     -5,   0,   1,   5,   8,   2,  15, -20,
    -10,   0,   0,   5,   6,   0,  20, -15,
    -10,   0,   0,   0,   0,   0,  25,  -5,
    -15,   0,  -5,  -5,  -5,  -5,   5,  -5,
      0,   0,   0,   0,   0,   0,   0,   0 
};
constexpr std::array<Score, kSquareCount> kEG_PawnPST = 
{
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
     10,  10,  10,  10,  10,  10,  10,  10,
     20,  20,  20,  22,  22,  20,  20,  20,
     35,  35,  38,  40,  40,  38,  35,  35,
     60,  60,  62,  65,  65,  62,  60,  60,
     90,  90,  90,  90,  90,  90,  90,  90,
      0,   0,   0,   0,   0,   0,   0,   0
};

constexpr std::array<Score, kSquareCount> kMG_KingPST = 
{
     20,  30,  10, -10, -10,   0,  30,  20,
     10,  10, -10, -20, -20, -10,  10,  10,
    -20, -25, -30, -35, -35, -30, -25, -20,
    -30, -35, -40, -45, -45, -40, -35, -30,
    -40, -45, -50, -55, -55, -50, -45, -40,
    -45, -50, -55, -60, -60, -55, -50, -45,
    -50, -55, -60, -60, -60, -60, -55, -50,
    -50, -55, -60, -60, -60, -60, -55, -50
};

constexpr std::array<Score, kSquareCount> kEG_KingPST =
{
    -40, -25, -15, -10, -10, -15, -25, -40,
    -25, -10,   0,   5,   5,   0, -10, -25,
    -15,   0,  15,  20,  20,  15,   0, -15,
    -10,   5,  20,  25,  25,  20,   5, -10,
    -10,   5,  20,  25,  25,  20,   5, -10,
    -15,   0,  15,  20,  20,  15,   0, -15,
    -25, -10,   0,   5,   5,   0, -10, -25,
    -40, -25, -15, -10, -10, -15, -25, -40
};




constexpr std::array<std::array<std::array<Score, kSquareCount>, kPieceCount>, 2> kPSTs
{{
    { kMG_QueenPST, kMG_RookPST, kMG_BishopPST, kMG_KnightPST, kMG_PawnPST, kMG_KingPST },
    { kEG_QueenPST, kEG_RookPST, kEG_BishopPST, kEG_KnightPST, kEG_PawnPST, kEG_KingPST }
}};

template <Piece piece, Phase phase>
inline Score GetPSTScore(Square sqr)
{
    return kPSTs[static_cast<size_t>(phase)][static_cast<size_t>(piece)][static_cast<size_t>(sqr)];
}

}  // namespace game_eval


constexpr std::array<std::array<Bitboard, kRowCount>, kColorCount> kRowsInfront =
{{  
    // white rows in front.
    {
        0xffffffffffffff00ULL,
        0xffffffffffff0000ULL,
        0xffffffffff000000ULL,
        0xffffffff00000000ULL,
        0xffffff0000000000ULL,
        0xffff000000000000ULL,
        0xff00000000000000ULL,
        0x0ULL,
    },
    {
        0x0ULL,
        0xffULL,
        0xffffULL,
        0xffffffULL,
        0xffffffffULL,
        0xffffffffffULL,
        0xffffffffffffULL,
        0xffffffffffffffULL
    }
}}; 

constexpr std::array<Bitboard, kColumnCount> kNearbyColumns = 
{
    kColumnsBitboards[static_cast<size_t>(Column::B)],
    kColumnsBitboards[static_cast<size_t>(Column::A)] | kColumnsBitboards[static_cast<size_t>(Column::C)],
    kColumnsBitboards[static_cast<size_t>(Column::B)] | kColumnsBitboards[static_cast<size_t>(Column::D)],
    kColumnsBitboards[static_cast<size_t>(Column::C)] | kColumnsBitboards[static_cast<size_t>(Column::E)],  
    kColumnsBitboards[static_cast<size_t>(Column::D)] | kColumnsBitboards[static_cast<size_t>(Column::F)],
    kColumnsBitboards[static_cast<size_t>(Column::E)] | kColumnsBitboards[static_cast<size_t>(Column::G)],
    kColumnsBitboards[static_cast<size_t>(Column::F)] | kColumnsBitboards[static_cast<size_t>(Column::H)],
    kColumnsBitboards[static_cast<size_t>(Column::G)]
};


// Column A = [0] to H.
// Nearby Columns plus self.
constexpr std::array<Bitboard, kColumnCount> kBandColumns = 
{
    kColumnsBitboards[static_cast<size_t>(Column::B)] | kColumnsBitboards[static_cast<size_t>(Column::A)] ,
    kColumnsBitboards[static_cast<size_t>(Column::A)] | kColumnsBitboards[static_cast<size_t>(Column::B)] | kColumnsBitboards[static_cast<size_t>(Column::C)],
    kColumnsBitboards[static_cast<size_t>(Column::B)] | kColumnsBitboards[static_cast<size_t>(Column::C)] | kColumnsBitboards[static_cast<size_t>(Column::D)],
    kColumnsBitboards[static_cast<size_t>(Column::C)] | kColumnsBitboards[static_cast<size_t>(Column::D)] | kColumnsBitboards[static_cast<size_t>(Column::E)],  
    kColumnsBitboards[static_cast<size_t>(Column::D)] | kColumnsBitboards[static_cast<size_t>(Column::E)] | kColumnsBitboards[static_cast<size_t>(Column::F)],
    kColumnsBitboards[static_cast<size_t>(Column::E)] | kColumnsBitboards[static_cast<size_t>(Column::F)] | kColumnsBitboards[static_cast<size_t>(Column::G)],
    kColumnsBitboards[static_cast<size_t>(Column::F)] | kColumnsBitboards[static_cast<size_t>(Column::G)] | kColumnsBitboards[static_cast<size_t>(Column::H)],
    kColumnsBitboards[static_cast<size_t>(Column::G)] | kColumnsBitboards[static_cast<size_t>(Column::H)]
};








namespace spsa_eval
{
enum class Phase : uint8_t { kMg, kEg };

inline Score kTempoBonus = 10;
inline Score kBishopPairBonus = 30;
inline Score kKnightPairPenalty = -8;
inline Score kRookPairPenalty = -20;
inline Score kTradeDownBonus = 10;
inline Score kNoPawnsPenalty = -30;
inline int kStartNonPawnMaterial = 6200;

// King Safety
// Per pawn on x rows in front it. So [0] = current row and is skipped, [1] = one row in front
constexpr int kPawnShieldRowsInfront{3};  // array size: must stay constexpr
inline std::array<Score, kPawnShieldRowsInfront> kPawnShieldBonus = {0, 5, 2};
constexpr int kPawnStormRowsInfront{5};  // array size: must stay constexpr
inline std::array<Score, kPawnStormRowsInfront> kPawnStormPenalty = {0, -2, -2, -1, -1};
inline Score kPawnShieldPenality = -7;


// Pawn structure
// Passed pawn
inline std::array<std::array<Score, kRowCount>, 2> kPassedPawnBonus =
{{
    // Middlegame
    { 0, 1, 3, 6, 3, 5, 8, 0 },
    // Endgame
    { 0, 6, 5, 6, 12, 15, 18, 0 },
}};


inline std::array<std::array<Score, kRowCount>, 2> kCandidatePassedPawnBonus =
{{
    // Middlegame
    { 0, -4, 1, 4, 6, 1, 2, 0 },
    // Endgame
    { 0, 8, 4, 5, 3, 4, 9, 0 },
}};


// Isolated pawn: no friendly pawns on adjacent files
inline std::array<Score, 2> kIsolatedPawnPenalty = { -4, -8 };  // { MG, EG }
// Doubled pawn: applied per extra pawn beyond the first on a file. Worse in EG (fewer pieces to compensate).

inline std::array<Score, 2> kDoubledPawnPenalty = { -8, -14 };  // { MG, EG }
// Backward pawn: stop square lacks pawn protection and is controlled by a sentry.
inline std::array<Score, 2> kBackwardPawnPenalty = { -5, -4 };  // { MG, EG }
// For how many islands, maximum there can only be 4 islands

inline std::array<std::array<Score, 9>, 2> kPawnIslandsPenalty =
{{
    // Middlegame
    { 0, 0, 1, -2, -5, -1, 0, 0, 0 },
    // Endgame
    { 0, 0, -3, 0, -5, -3, 0, 0, 0 },
}};
inline std::array<std::array<Score, 5>, 2> kPawnMajorityBonus =
{{
    // Middlegame
    { 0, 2, 2, 1, -1 },
    // Endgame
    { 0, 3, 2, 1, 10 },
}};
inline Bitboard kKingsideMask = kColumnsBitboards[static_cast<size_t>(Column::F)] | 
                                   kColumnsBitboards[static_cast<size_t>(Column::G)] | 
                                   kColumnsBitboards[static_cast<size_t>(Column::H)];
inline Bitboard kQueensideMask = kColumnsBitboards[static_cast<size_t>(Column::A)] | 
                                    kColumnsBitboards[static_cast<size_t>(Column::B)] | 
                                    kColumnsBitboards[static_cast<size_t>(Column::C)] | 
                                    kColumnsBitboards[static_cast<size_t>(Column::D)];
// Indexed by the rank of the defending pawn.
inline std::array<std::array<Score, kRowCount>, 2> kChainPawnBonus =
{{
    // Middlegame
    { 0, 5, 2, 3, 5, -1, 2, 0 },
    // Endgame
    { 0, 5, 1, 1, 1, 12, 1, 0 },
}};
// Phalanx: indexed by the shared rank. Counted once per pawn, so a side-by-side pair
// scores this twice. Zero on rank 2 so the untouched starting wall earns nothing.
inline std::array<std::array<Score, kRowCount>, 2> kPhalanxPawnBonus =
{{
    // Middlegame
    { 0, -4, 2, 4, 3, 1, 0, 0 },
    // Endgame
    { 0, 3, 3, 2, 3, 2, 1, 0 },
}};

inline std::array<Score, kPieceCount> kMG_VisionBonuses = 
{
    1, // queen
    2, // rook
    4, // bishop
    4, // knight
    0, // pawn
    -1  // king
};

inline std::array<Score, kPieceCount> kEG_VisionBonuses = 
{
    2, // queen
    4, // rook
    5, // bishop
    4, // knight
    0, // pawn
    0  // king
};

inline std::array<Score, kPieceCount> kPieceValues = 
{
    900, // queen
    500, // rook
    300, // bishop
    300, // knight
    100, // pawn
    0    // king
};

inline std::array<Score, kSquareCount> kMG_QueenPST = 
{
    -10,  -5,  -5,   0,   0,  -5,  -5, -10,
     -5,   0,   5,   5,   5,   5,   0,  -5,
     -5,   5,   5,   5,   5,   5,   5,  -5,
      0,   5,   5,   8,   8,   5,   5,  -5,
     -5,   5,   5,   8,   8,   5,   5,  -5,
     -5,   5,   5,   5,   5,   5,   5,  -5,
     -5,   0,   5,   5,   5,   5,   0,  -5,
    -10,  -5,  -5,   0,   0,  -5,  -5, -10
};

inline std::array<Score, kSquareCount> kEG_QueenPST = 
{
    -20, -15, -10,  -5,  -5, -10, -15, -20,
    -15,  -5,   0,   0,   0,   0,  -5, -15,
    -10,   0,   5,  10,  10,   5,   0, -10,
     -5,   0,  10,  15,  15,  10,   0,  -5,
     -5,   0,  10,  15,  15,  10,   0,  -5,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -15,  -5,   0,   0,   0,   0,  -5, -15,
    -20, -15, -10,  -5,  -5, -10, -15, -20
};

inline std::array<Score, kSquareCount> kMG_RookPST = 
{
     -5,   0,   5,  10,  10,   5,   0,  -5,
     -5,   0,   0,   5,   5,   0,   0,  -5,
     -5,   0,   0,   5,   5,   0,   0,  -5,
     -5,   0,   0,   5,   5,   0,   0,  -5,
     -5,   0,   0,   5,   5,   0,   0,  -5,
     -5,   0,   0,   5,   5,   0,   0,  -5,
     15,  20,  20,  20,  20,  20,  20,  15,
      0,   0,   0,   5,   5,   0,   0,   0
};

inline std::array<Score, kSquareCount> kEG_RookPST = 
{
     -5,  -3,   0,   0,   0,   0,  -3,  -5,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      3,   3,   3,   3,   3,   3,   3,   3,
      5,   5,   5,   5,   5,   5,   5,   5,
      8,   8,   8,   8,   8,   8,   8,   8,
     12,  12,  12,  12,  12,  12,  12,  12,
      5,   5,   5,   5,   5,   5,   5,   5
};

inline std::array<Score, kSquareCount> kMG_BishopPST = 
{
    -15,  -5, -10,  -5,  -5, -10,  -5, -15,
     -5,  10,   5,   5,   5,   5,  10,  -5,
     -5,   8,  10,  10,  10,  10,   8,  -5,
     -5,   5,  10,  15,  15,  10,   5,  -5,
     -5,   5,  10,  15,  15,  10,   5,  -5,
     -5,   8,  10,  10,  10,  10,   8,  -5,
     -5,  10,   5,   5,   5,   5,  10,  -5,
    -15,  -5, -10,  -5,  -5, -10,  -5, -15
};

inline std::array<Score, kSquareCount> kEG_BishopPST = 
{
    -15, -10,  -8,  -5,  -5,  -8, -10, -15,
    -10,   0,   0,   3,   3,   0,   0, -10,
     -8,   0,   5,   8,   8,   5,   0,  -8,
     -5,   3,   8,  10,  10,   8,   3,  -5,
     -5,   3,   8,  10,  10,   8,   3,  -5,
     -8,   0,   5,   8,   8,   5,   0,  -8,
    -10,   0,   0,   3,   3,   0,   0, -10,
    -15, -10,  -8,  -5,  -5,  -8, -10, -15
};

inline std::array<Score, kSquareCount> kMG_KnightPST = 
{
    -50, -30, -15, -10, -10, -15, -30, -50,
    -30, -10,   5,  10,  10,   5, -10, -30,
    -15,   5,  15,  18,  18,  15,   5, -15,
    -10,  10,  18,  22,  22,  18,  10, -10,
    -10,  10,  18,  22,  22,  18,  10, -10,
    -15,   5,  15,  18,  18,  15,   5, -15,
    -30, -10,   5,  10,  10,   5, -10, -30,
    -50, -30, -15, -10, -10, -15, -30, -50
};

inline std::array<Score, kSquareCount> kEG_KnightPST = 
{
    -40, -25, -15, -10, -10, -15, -25, -40,
    -25, -10,   0,   5,   5,   0, -10, -25,
    -15,   0,   8,  12,  12,   8,   0, -15,
    -10,   5,  12,  15,  15,  12,   5, -10,
    -10,   5,  12,  15,  15,  12,   5, -10,
    -15,   0,   8,  12,  12,   8,   0, -15,
    -25, -10,   0,   5,   5,   0, -10, -25,
    -40, -25, -15, -10, -10, -15, -25, -40
};


inline std::array<Score, kSquareCount> kMG_PawnPST = 
{
      0,   0,   0,   0,   0,   0,   0,   0,
     20,  30,  15,  20,  15,  30,  25,  -5,
      0,   0,   7,   5,  10,  10,  10, -10,
     -5,   0,   1,   5,   8,   2,  15, -20,
    -10,   0,   0,   5,   6,   0,  20, -15,
    -10,   0,   0,   0,   0,   0,  25,  -5,
    -15,   0,  -5,  -5,  -5,  -5,   5,  -5,
      0,   0,   0,   0,   0,   0,   0,   0 
};
inline std::array<Score, kSquareCount> kEG_PawnPST = 
{
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
     10,  10,  10,  10,  10,  10,  10,  10,
     20,  20,  20,  22,  22,  20,  20,  20,
     35,  35,  38,  40,  40,  38,  35,  35,
     60,  60,  62,  65,  65,  62,  60,  60,
     90,  90,  90,  90,  90,  90,  90,  90,
      0,   0,   0,   0,   0,   0,   0,   0
};

inline std::array<Score, kSquareCount> kMG_KingPST = 
{
     20,  30,  10, -10, -10,   0,  30,  20,
     10,  10, -10, -20, -20, -10,  10,  10,
    -20, -25, -30, -35, -35, -30, -25, -20,
    -30, -35, -40, -45, -45, -40, -35, -30,
    -40, -45, -50, -55, -55, -50, -45, -40,
    -45, -50, -55, -60, -60, -55, -50, -45,
    -50, -55, -60, -60, -60, -60, -55, -50,
    -50, -55, -60, -60, -60, -60, -55, -50
};

inline std::array<Score, kSquareCount> kEG_KingPST =
{
    -40, -25, -15, -10, -10, -15, -25, -40,
    -25, -10,   0,   5,   5,   0, -10, -25,
    -15,   0,  15,  20,  20,  15,   0, -15,
    -10,   5,  20,  25,  25,  20,   5, -10,
    -10,   5,  20,  25,  25,  20,   5, -10,
    -15,   0,  15,  20,  20,  15,   0, -15,
    -25, -10,   0,   5,   5,   0, -10, -25,
    -40, -25, -15, -10, -10, -15, -25, -40
};




// Pointers, not copies: a write to kMG_QueenPST has to be what the search reads.
using PSTable = std::array<Score, kSquareCount>;
inline std::array<std::array<PSTable*, kPieceCount>, 2> kPSTs
{{
    { &kMG_QueenPST, &kMG_RookPST, &kMG_BishopPST, &kMG_KnightPST, &kMG_PawnPST, &kMG_KingPST },
    { &kEG_QueenPST, &kEG_RookPST, &kEG_BishopPST, &kEG_KnightPST, &kEG_PawnPST, &kEG_KingPST }
}};

template <Piece piece, Phase phase>
inline Score GetPSTScore(Square sqr)
{
    return (*kPSTs[static_cast<size_t>(phase)][static_cast<size_t>(piece)])[static_cast<size_t>(sqr)];
}

}  // namespace spsa_eval