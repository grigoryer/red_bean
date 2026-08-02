#pragma once

#include <cstddef>
#include "move.hpp"
#include "global.hpp"
#include "search_globals.hpp"
#include <array>



inline Score ScoreToTable(Score score, uint8_t ply)
{
    if (score < -kMateScoreThreshold) { return score - ply; }
    if (score >  kMateScoreThreshold) { return score + ply; }
    return score;
}

inline Score TableToScore(Score score, uint8_t ply)
{
    if (score < -kMateScoreThreshold) { return score + ply; } 
    if (score >  kMateScoreThreshold) { return score - ply; }
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

    void AddEntry(TEntry entry);
    TEntry GetEntry(Hash hash);
    void ProfileTableSpread();
    void ClearTT();
};