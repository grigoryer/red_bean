#include "tt.hpp"
#include "search_globals.hpp"
#include <cstdint>
#include <iostream>
#include <limits>

TEntry::TEntry(Hash hash, Score score, Move refutation_move, uint8_t depth, NodeType type)
    : hash_(hash),
      score_(score),
      refutation_move_(refutation_move),
      depth_(depth),
      type_(type)
{ }


TEntry* TTable::AddEntry(TEntry entry)
{
    size_t index = entry.hash_ % kTTableSize;
    auto* bucket_itr = array_[index].bucket_.begin();
    auto* worst_entry = bucket_itr;
    uint8_t worst_depth = std::numeric_limits<uint8_t>::max();

    while (bucket_itr != array_[index].bucket_.end())
    {
        // if already have a copy of it.
        if (bucket_itr->hash_ == entry.hash_)
        {
            if (bucket_itr->depth_ >= entry.depth_)
            {
                return bucket_itr;
            }
            
            worst_entry = bucket_itr;
            break;
        }

        // if its empty immediatly return.
        if (bucket_itr->hash_ == 0) 
        { 
            worst_entry = bucket_itr; 
            break;
        };

        if (bucket_itr->depth_ < worst_depth)
        {
            worst_depth = bucket_itr->depth_;
            worst_entry = bucket_itr;
        }
        bucket_itr++;
    }
    
    *worst_entry = entry;

    return worst_entry;
}

TEntry* TTable::GetEntry(Hash hash)
{
    size_t index = hash % kTTableSize;

    for (auto& entry : array_[index].bucket_)
    {
        if (entry.hash_ == hash) { node_hits++; return &entry; }
    }

    return nullptr;
};
    


void TTable::ProfileTableSpread()
{
    uint64_t non_null_entries{};
    uint64_t histogram[kTBucketSize + 1]{};  // indices 0..kBucketSize

    for (auto& bucket : array_)
    {
        int filled{};
        for (const auto& entry : bucket.bucket_)
        {
            if (entry.hash_ != 0)
            {
                ++non_null_entries;
                ++filled;
            }
        }
        ++histogram[filled];
    }

    const uint64_t total_slots = static_cast<uint64_t>(kTTableSize) * kTBucketSize;
    const double   fill_factor = 100 * static_cast<double>(non_null_entries) / total_slots;

    std::cout << "\nFill Factor:            " << fill_factor
              << "\nNon Null Entries:       " << non_null_entries
              << "\n--- Bucket Occupancy Histogram ---";

    for (size_t i = 0; i <= kTBucketSize; ++i)
    {
        const double pct = static_cast<double>(histogram[i]) / kTTableSize * 100.0;
        std::cout << "\n  " << i << " filled: " << histogram[i]
                  << " buckets (" << pct << "%)";
    }
    std::cout << std::endl;
}