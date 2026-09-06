#include "tt.hpp"
#include <cstdint>
#include <limits>
#include <algorithm>

#if defined(__APPLE__)
#include <malloc/malloc.h>
#endif

// data_ must precede key_: key_ is derived from it
TEntry::TEntry(Hash hash, Score score, Move refutation_move, uint8_t depth, NodeType type, uint8_t age)
    : data_((static_cast<uint64_t>(static_cast<uint16_t>(score)) << kScoreShift)
          | (static_cast<uint64_t>(refutation_move.Raw()) << kMoveShift)
          | (static_cast<uint64_t>(depth) << kDepthShift)
          | (static_cast<uint64_t>(age) << kAgeShift)
          | (static_cast<uint64_t>(type) << kTypeShift)), key_(hash ^ data_)
{ }


void TTable::Resize(size_t mb)
{
    mb = std::clamp(mb, kMinHashMB, kMaxHashMB);

    // Largest power-of-two bucket count that fits the request, so indexing
    // stays a mask. Rounding down means we never exceed what was asked for.
    size_t buckets = 1;
    while ((buckets * 2 * sizeof(TBucket)) <= (mb * 1024 * 1024)) { buckets *= 2; }

    std::vector<TBucket>().swap(array_);

    #if defined(__APPLE__) // free up on mac
        malloc_zone_pressure_relief(nullptr, 0);
    #endif
        
    array_.resize(buckets);   // single allocation, value-initialised to key_ == 0
    bucket_mask_ = buckets - 1;
}

void TTable::AddEntry(TEntry entry)
{
    // current age is entry.age, since it is the latest addition it has to be newest age.
    // strategy is to replace ag eif it is 2 away otherwise replace depth.
    size_t index = entry.GetKey() & bucket_mask_;

    auto* end = array_[index].bucket_.end();
    auto* bucket_itr = array_[index].bucket_.begin();

    auto* worst_depth_entry = bucket_itr;
    auto* worst_age_entry = bucket_itr;

    uint8_t worst_depth = std::numeric_limits<uint8_t>::max();
    uint8_t worst_age = std::numeric_limits<uint8_t>::max();

    while (bucket_itr != end)
    {
        // if already have a copy of it keep the higher depth
        if (bucket_itr->GetKey() == entry.GetKey())
        {
            if (bucket_itr->GetDepth() >= entry.GetDepth())
            {
                return;
            }

            *bucket_itr = entry; // immediatly replace and leave.
            return;
        }

        // if its empty immediatly return it as no need to look, only for compulsory misses.
        if (bucket_itr->IsEmpty())
        {
            *bucket_itr = entry; // immediatly replace and leave.
            return;
        };

        // Actual replacement logic
        if (bucket_itr->GetAge() <= worst_age) // Keep track of worst age.
        {
            worst_age = bucket_itr->GetAge();
            worst_age_entry = bucket_itr;
        }

        // keep track of worst depth
        if (bucket_itr->GetDepth() <= worst_depth)
        {
            worst_depth = bucket_itr->GetDepth();
            worst_depth_entry = bucket_itr;
        }
        bucket_itr++;
    }

    // if the worst age is more than 2 away replace if not then replace worst depth.
    if (static_cast<int>(worst_age) <= (static_cast<int>(entry.GetAge()) - static_cast<int>(kAgeMargin)))
    {
        *worst_age_entry = entry;
    }
    else 
    {
        *worst_depth_entry = entry;
    }
}

TEntry TTable::GetEntry(Hash hash)
{
    size_t index = hash & bucket_mask_;

    for (const auto& slot : array_[index].bucket_)
    {
        TEntry copy = slot; // read once; verify the same bytes we return (other threads may overwrite the slot)
        if (copy.GetKey() == hash)
        {
            return copy;
        }
    }

    return TEntry{};
};


void TTable::ResetAges()
{
    for (auto& bucket : array_)
    {
        for (auto& entry : bucket.bucket_) { entry.SetAge(0); }
    }
}


void TTable::ClearTT()
{
    std::fill(array_.begin(), array_.end(), TBucket{});
}