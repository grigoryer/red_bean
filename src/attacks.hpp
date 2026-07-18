#pragma once

#include "global.hpp"
#include <iostream>

// Attack table for slider and leaper pieces

namespace attacks
{
    Bitboard GetPawnAttack(Color color, Square sqr);
    Bitboard GetKingAttack(Square sqr);
    Bitboard GetKnightAttack(Square sqr);
    Bitboard GetBishopAttack(Bitboard occ, Square sqr);
    Bitboard GetRookAttack(Bitboard occ, Square sqr);
    Bitboard GetQueenAttack(Bitboard occ, Square sqr);

    template<Piece piece>
    Bitboard getPieceAttack(Bitboard occ, Square sqr, Color color)
    {
        if constexpr (piece == Piece::kKing)
        {
            return GetKingAttack(sqr);
        }
        else if constexpr (piece == Piece::kPawn)
        {
            return GetPawnAttack(color, sqr);
        }
        else if constexpr (piece == Piece::kKnight)
        {
            return GetKnightAttack(sqr);
        }
        else if constexpr (piece == Piece::kBishop)
        {
            return GetBishopAttack(occ, sqr);
        }
        else if constexpr (piece == Piece::kRook)
        {
            return GetRookAttack(occ, sqr);
        }
        else if constexpr (piece == Piece::kQueen)
        {
            return GetQueenAttack(occ, sqr);
        }

        std::cerr << "Cant get Attack of None Type";
        std::terminate();
    }
} // namespace attacks

