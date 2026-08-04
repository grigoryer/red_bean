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


constexpr size_t kTBucketSize = 4; // prev was cache line size / Tentry size, changed back to 4 since change didnt push sprt 
enum class NodeType : uint8_t
{
    EXACT,
    UPPER,
    LOWER
};

// Improving
constexpr int kEvalStackOffset = 10;

// Quesience Search
constexpr uint8_t kQuesDepth = 7;

// Late Move Reduction
constexpr double kLMR_Base = 0.75;
constexpr double kLMR_Divisor = 2.25;
constexpr int kLMR_HistReduction = 2;

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
// Move             Score
// TT MOVE          12'000
// Good captures    11'000 -- 2'900   (all except major pieces taking pawn) 
// killers          2'850  -- 2'750
// Med taking pawns 2'700
// quiets           2'625  -- -2'625
// bad captures     -2'625 -- -14'625


// Move ordering bonuses.
constexpr std::array<MoveScore, kPieceCount> kPieceMoveWorth { 900, 500, 300, 300, 100, 0 };
constexpr MoveScore kRefutationMoveBoost = 12000; // if hash move boost it
constexpr MoveScore kCastleBoost = 0;     // Boost castling after all types of captures.
constexpr MoveScore kKiller1Boost = 2850;   // Killer move 1 above the weak captures of major piece taking a pawn, but not pawn takes pawn
constexpr MoveScore kKiller2Boost = 2750;   

// History Heuristics, max queit move = 2625. add all of these together.
constexpr MoveScore kHistoryMoveBoostCeiling = 1500;
constexpr MoveScore kHistoryPly1BoostCeiling = kHistoryMoveBoostCeiling / 2;
constexpr MoveScore kHistoryPly2BoostCeiling = kHistoryMoveBoostCeiling / 4;

// inside range of history.
constexpr MoveScore kHistoryMax = 1 << 13;
static_assert(kHistoryMax + (kMaxSearchDepth * kMaxSearchDepth) <= std::numeric_limits<MoveScore>::max(), "history entry can overflow between aging passes");

// SEE
constexpr MoveScore kBadCapturePenalty = -12000 - (kHistoryMoveBoostCeiling + kHistoryPly1BoostCeiling + kHistoryPly2BoostCeiling);

// MVV LVA
// [attacker][victim]
constexpr std::array<std::array<MoveScore, kPieceCount + 1>, kPieceCount> kMoveOrderingTable
{{  // victims: Q  R  B  N  P  K None
    { 10100, 6100, 4100, 4100, 2100, 0, 0 },  // kQueen
    { 10500, 6500, 4500, 4500, 2500, 0, 0 },  // kRook
    { 10700, 6700, 4700, 4700, 2700, 0, 0 },  // kBishop
    { 10700, 6700, 4700, 4700, 2700, 0, 0 },  // kKnight
    { 10900, 6900, 4900, 4900, 2900, 0, 0 },  // kPawn
    { 11000, 7000, 5000, 5000, 3000, 0, 0 },  // kKing
}};

