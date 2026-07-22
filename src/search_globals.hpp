#pragma once

#include "global.hpp"
#include <cstddef>
#include <cstdint>

using Score = int32_t;
using MoveScore = uint16_t;

constexpr size_t kTTableSize = 1 << 20;
constexpr size_t kTBucketSize = 1 << 2;


enum class NodeType : uint8_t
{
    EXACT,
    UPPER,
    LOWER
};

constexpr std::array<MoveScore, kPieceCount> kPieceWorth { 900, 500, 300, 300, 100, 0 };


constexpr MoveScore kRefutationMoveBoost = 10000;

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