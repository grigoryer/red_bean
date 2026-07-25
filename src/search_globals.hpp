#pragma once

#include "global.hpp"
#include <cstddef>
#include <cstdint>

using Score = int16_t;
using MoveScore = uint16_t;

constexpr Score kPosInf = std::numeric_limits<Score>::max() - 3000;
constexpr Score kNegInf = -kPosInf;

constexpr Score kMateScore = kNegInf + 1000;
constexpr Score kMateThreshold = kMateScore + 400;
constexpr Score kDrawScore = -700;

constexpr int kMaxPly = 64;
constexpr size_t kMaxSearchDepth{kMaxPly};
constexpr size_t kTTableSize = 1 << 20;
constexpr size_t kTBucketSize = 1 << 2;

constexpr uint8_t kQuesDepth = 5;

enum class NodeType : uint8_t
{
    EXACT,
    UPPER,
    LOWER
};

constexpr std::array<Score, kPieceCount> kScorePieceWorth { 900, 500, 300, 300, 100, 0 };
constexpr std::array<MoveScore, kPieceCount> kPieceWorth { 900, 500, 300, 300, 100, 0 };

constexpr MoveScore kCastleBoost = 900; // after all types of captures.
constexpr MoveScore kRefutationMoveBoost = 10000;
constexpr MoveScore kKiller1Boost = 2000;
constexpr MoveScore kKiller2Boost = 1900;

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