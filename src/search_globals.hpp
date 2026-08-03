#pragma once

#include "global.hpp"
#include <cstddef>
#include <cstdint>

using Score = int16_t;
using MoveScore = int16_t;

//Main Alpha Beta
constexpr Score kPosInf = std::numeric_limits<Score>::max() - 3000;
constexpr Score kNegInf = -kPosInf;
constexpr Score kMateScore = kNegInf + 1000;
constexpr Score kMateScoreThreshold = -kMateScore - 1000;
constexpr Score kDrawScore = -50;

// Depth and Iterative Deepening
constexpr int kMaxPly = 64;
constexpr size_t kMaxSearchDepth{kMaxPly};

// Transposition table 
// Default sizing.
constexpr size_t kDefaultHashMB = 128;
constexpr size_t kMinHashMB = 1;
constexpr size_t kMaxHashMB = 1024;


constexpr size_t kTBucketSize = 128 / 16; // cache line size / Tentry size
enum class NodeType : uint8_t
{
    EXACT,
    UPPER,
    LOWER
};

// Quesience Search
constexpr uint8_t kQuesDepth = 7;

// Late Move Reduction
constexpr double kLMR_Base = 0.75;
constexpr double kLMR_Divisor = 2.25;

// Late Move Pruning
constexpr uint8_t kLMP_MaxDepth = 6;
constexpr int kLMP_Base = 3;

// Reverse Futlity Prunning
constexpr uint8_t kRFP_MaxDepth = 6; // depth and which we can check for rfp, if our remaining depth is too high we cant trust it 100%.
constexpr Score kRFP_Margin = 80; // for how deep we prune we increase the margin, so depth * margin. 

// Futility Prunning
constexpr Score kFutilityMargin = 100;
constexpr uint8_t kFutilityMaxDepth = 3; // depth


/// MOVE OREDERING

// Move ordering bonuses.
constexpr std::array<MoveScore, kPieceCount> kPieceMoveWorth { 900, 500, 300, 300, 100, 0 };
constexpr MoveScore kRefutationMoveBoost = 10000; // if hash move boost it
constexpr MoveScore kCastleBoost = 900; // Boost castling after all types of captures.
constexpr MoveScore kKiller1Boost = 2000; // Killer move 1 above the weak captures of major piece taking a pawn, but not pawn takes pawn
constexpr MoveScore kKiller2Boost = 1900; // Cjamge to 1750 1700 after test

// History Heuristics
constexpr MoveScore kHistoryMoveBoostCeiling = 1500;

constexpr MoveScore kHistoryMax = 1 << 13;
static_assert(kHistoryMax + (kMaxSearchDepth * kMaxSearchDepth) <= std::numeric_limits<MoveScore>::max(), "history entry can overflow between aging passes");

// SEE
constexpr MoveScore kBadCapturePenalty = -10000;

// MVV LVA
// [attacker][victim]
constexpr std::array<std::array<MoveScore, kPieceCount + 1>, kPieceCount> kMoveOrderingTable
{{  // victims: Q  R  B  N  P  K None
    { 9000, 5000, 3000, 3000, 1000, 0, 0 },  // kQueen
    { 9400, 5400, 3400, 3400, 1400, 0, 0 },  // kRook
    { 9600, 5600, 3600, 3600, 1600, 0, 0 },  // kBishop
    { 9600, 5600, 3600, 3600, 1600, 0, 0 },  // kKnight
    { 9800, 5800, 3800, 3800, 1800, 0, 0 },  // kPawn
    { 9900, 5900, 3900, 3900, 1900, 0, 0 },  // kKing
}};


