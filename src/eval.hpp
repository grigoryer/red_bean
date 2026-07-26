#pragma once 

#include "board.hpp"
#include "global.hpp"
#include "search_globals.hpp"


constexpr std::array<Bitboard, kColumnCount> kNearbyColumns = 
{
    kColumnsBitboards[static_cast<size_t>(Column::B)],
    kColumnsBitboards[static_cast<size_t>(Column::A)] | kColumnsBitboards[static_cast<size_t>(Column::C)],
    kColumnsBitboards[static_cast<size_t>(Column::B)] | kColumnsBitboards[static_cast<size_t>(Column::D)],
    kColumnsBitboards[static_cast<size_t>(Column::C)] | kColumnsBitboards[static_cast<size_t>(Column::E)],  
    kColumnsBitboards[static_cast<size_t>(Column::D)] | kColumnsBitboards[static_cast<size_t>(Column::F)],
    kColumnsBitboards[static_cast<size_t>(Column::E)] | kColumnsBitboards[static_cast<size_t>(Column::G)],
    kColumnsBitboards[static_cast<size_t>(Column::F)] | kColumnsBitboards[static_cast<size_t>(Column::H)],
    kColumnsBitboards[static_cast<size_t>(Column::G)]
};

namespace evaluation
{
    Score Evaluate(Board& board);
}