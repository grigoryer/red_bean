#pragma once

#include <cstddef>
#include "move.hpp"
#include "global.hpp"
#include "search_constants.hpp"
#include <array>
#include <vector>

inline Score ScoreToTable(Score score, uint8_t ply)
{
    if (score < -kMateScoreThreshold) { return score - ply; }
    if (score > kMateScoreThreshold) { return score + ply; }
    return score;
}

inline Score TableToScore(Score score, uint8_t ply)
{
    if (score < -kMateScoreThreshold) { return score + ply; } 
    if (score > kMateScoreThreshold) { return score - ply; }
    return score;
}

inline uint64_t CreateData(Hash hash, Score score, Move refutation_move, uint8_t depth, NodeType type, uint8_t age);

class TEntry
{
public:
    TEntry() = default;
    TEntry(Hash hash, Score score, Move refutation_move, uint8_t depth, NodeType type, uint8_t age);

    [[nodiscard]] constexpr Hash GetKey() const { return key_ ^ data_; }
    [[nodiscard]] constexpr bool IsEmpty() const { return key_ == 0 && data_ == 0; }
    [[nodiscard]] constexpr uint64_t GetData() const { return data_; }

    [[nodiscard]] constexpr Score GetScore() const
    { return static_cast<Score>(static_cast<int16_t>((data_ >> kScoreShift) & kScoreMask)); }

    [[nodiscard]] constexpr Move GetMove() const
    { return Move::FromRaw(static_cast<uint16_t>((data_ >> kMoveShift) & kMoveMask)); }

    [[nodiscard]] constexpr uint8_t GetDepth() const
    { return static_cast<uint8_t>((data_ >> kDepthShift) & kDepthMask); }

    [[nodiscard]] constexpr uint8_t GetAge() const
    { return static_cast<uint8_t>((data_ >> kAgeShift) & kAgeMask); }

    [[nodiscard]] constexpr NodeType GetType() const
    { return static_cast<NodeType>((data_ >> kTypeShift) & kTypeMask); }

    // Only mutator: the age wrap sweep in FindBestMove stales every entry in place.
    constexpr void SetAge(uint8_t age)
    {
        const Hash hash = GetKey(); // key_ is hash ^ data_, so it must be rebuilt after data_ changes
        data_ = (data_ & ~(kAgeMask << kAgeShift)) | (static_cast<uint64_t>(age) << kAgeShift);
        key_ = hash ^ data_;
    }

private:
    static constexpr uint8_t kScoreShift = 0;   // 16 bits: Score
    static constexpr uint8_t kMoveShift = 16;  // 16 bits: Move::Raw()
    static constexpr uint8_t kDepthShift = 32;  // 8 bits
    static constexpr uint8_t kAgeShift = 40;  // 8 bits
    static constexpr uint8_t kTypeShift = 48;  // 2 bits: NodeType. 50-63 free.

    static constexpr uint64_t kScoreMask = 0xFFFF;
    static constexpr uint64_t kMoveMask = 0xFFFF;
    static constexpr uint64_t kDepthMask = 0xFF;
    static constexpr uint64_t kAgeMask = 0xFF;
    static constexpr uint64_t kTypeMask = 0x3;

    // data_ must precede key_: key_ is derived from it
    uint64_t data_{0};  // 8 bytes
    Hash key_{0};       // 8 bytes
};

class alignas(sizeof(TEntry) * kTBucketSize) TBucket
{
public:
    std::array<TEntry, kTBucketSize> bucket_;
};


class TTable
{
public:
    std::vector<TBucket> array_;
    size_t bucket_mask_{};   // count - 1; the count is always a power of two

    explicit TTable(size_t mb = kDefaultHashMB) { Resize(mb); }
    void Resize(size_t mb);

    void AddEntry(TEntry entry);
    TEntry GetEntry(Hash hash);
    void ResetAges();
    void ClearTT();

    void PrefetchEntry(Hash hash) const
    {
        __builtin_prefetch(&array_[hash & bucket_mask_]);
    }
};