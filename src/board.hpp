#pragma once

#include "global.hpp"
#include <cstddef>


class Status
{


};

class Board
{
private:

    std::array<Bitboard, kPieceCount> piece_bb_{};      // Bitboard for each piece
    std::array<Bitboard, kColorCount + 1> color_bb_{};  // Bitboard for each occupancy, + 1 for both sides, reduntand but will save lots of runtime calls later.
    std::array<Piece, kSquareCount> piece_list_{};      // For each square we can keep track exactly which piece is there.

public:

    [[nodiscard]] Bitboard GetPieceBB(const Piece piece) const
    {
        return piece_bb_.at(static_cast<size_t>(piece));
    }

    [[nodiscard]] Bitboard GetColorBB(const Color color) const
    {
        return color_bb_.at(static_cast<size_t>(color));
    }

    [[nodiscard]] Piece GetPieceOnSquare(const Square sqr) const
    {
        return piece_list_.at(ToIndex(sqr));
    }
    
    void SetPieceBB(const Piece piece, const Square sqr)
    {
        SetBit(piece_bb_.at(static_cast<size_t>(piece)), sqr);
    }

    void SetColorBB(const Color color, const Square sqr)
    {
        SetBit(color_bb_.at(static_cast<size_t>(color)), sqr);
    }

    void SetPieceOnSquare(const Piece piece, const Square sqr)
    {
        piece_list_.at(ToIndex(sqr)) = piece;
    }
    
    void ParseFenString(const std::string& fen_string);
};