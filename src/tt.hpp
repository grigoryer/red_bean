#pragma once

#include <cstddef>
#include "move.hpp"
#include "global.hpp"
#include "search_globals.hpp"
#include <array>


// Search-relative (distance from root) -> TT-relative (distance from this node).
[[nodiscard]] constexpr Score ScoreToTT(Score score, uint8_t ply)
{
    if (score >= kMateThreshold) { return score + ply; }
    if (score <= -kMateThreshold) { return score - ply; }
    return score;
}

// TT-relative -> search-relative.
[[nodiscard]] constexpr Score ScoreFromTT(Score score, uint8_t ply)
{
    if (score >= kMateThreshold) { return score - ply; }
    if (score <= -kMateThreshold) { return score + ply; }
    return score;
}

class TEntry
{
public:
    Hash hash_{0};
    Score score_{};
    Move refutation_move_;
    uint8_t depth_{};
    NodeType type_{};
    uint8_t age_{};

    TEntry() = default;
    TEntry(Hash hash, Score score, Move refutation_move, uint8_t depth, NodeType type, uint8_t age);
};

class TBucket
{
public:
    std::array<TEntry, kTBucketSize> bucket_;
};


class TTable
{
    std::array<TBucket, kTTableSize> array_;

public:
    uint64_t node_hits{0};
    uint64_t node_requests{0};

    TEntry* AddEntry(TEntry entry);
    TEntry* GetEntry(Hash hash);
    void ProfileTableSpread();
    void ClearTT();
};