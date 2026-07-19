#pragma once
#include "board.hpp"
#include "global.hpp"

// in do move only will need to recompute on same hash.

namespace between
{
    void SetCheckersAndBlockers(Board& board, Status& status);
    Bitboard GetRayBetweenSquares(Square sqr_1, Square sqr_2);
    Bitboard GetRayThroughSquares(Square sqr_1, Square sqr_2);
} //namespace between