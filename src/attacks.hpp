#pragma once

#include "global.hpp"

// Attack table for slider and leaper pieces



namespace attacks
{
    
    Bitboard GetPawnAttack(Color color, Square sqr);
    Bitboard GetKingAttack(Square sqr);
    Bitboard GetKnightAttack(Square sqr);
    Bitboard GetBishopAttack(Bitboard occ, Square sqr);
    Bitboard GetRookAttack(Bitboard occ, Square sqr);
    Bitboard GetQueenAttack(Bitboard occ, Square sqr);

} // namespace attacks

