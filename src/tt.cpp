#include "tt.hpp"
#include "search_globals.hpp"
#include <cstdint>
#include <limits>
#include <algorithm>

#if defined(__APPLE__)
#include <malloc/malloc.h>
#endif

TEntry::TEntry(Hash hash, Score score, Move refutation_move, uint8_t depth, NodeType type, uint8_t age)
    : hash_(hash),
      score_(score),
      refutation_move_(refutation_move),
      depth_(depth),
      type_(type),
      age_(age % 256)
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
        
    array_.resize(buckets);   // single allocation, value-initialised to hash_ == 0
    bucket_mask_ = buckets - 1;

    node_hits = 0;
    node_requests = 0;
}

void TTable::AddEntry(TEntry entry)
{
    size_t index = entry.hash_ & bucket_mask_;
    auto* bucket_itr = array_[index].bucket_.begin();
    auto* worst_entry = bucket_itr;
    uint8_t worst_age = std::numeric_limits<uint8_t>::max();

    while (bucket_itr != array_[index].bucket_.end())
    {
        // if already have a copy of it keep the higher depth
        if (bucket_itr->hash_ == entry.hash_)
        {
            if (bucket_itr->depth_ >= entry.depth_)
            {
                return;
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

        if (bucket_itr->age_ <= worst_age)
        {
            worst_age = bucket_itr->age_;
            worst_entry = bucket_itr;
        }
        bucket_itr++;
    }
    
    *worst_entry = entry;
}


TEntry TTable::GetEntry(Hash hash)
{
    size_t index = hash & bucket_mask_;;

    node_requests++;

    for (auto& entry : array_[index].bucket_)
    {
        if (entry.hash_ == hash) { node_hits++; return entry; }
    }

    return TEntry{};
};


void TTable::ClearTT()
{
    std::fill(array_.begin(), array_.end(), TBucket{});
    node_hits = 0;
    node_requests = 0;
}