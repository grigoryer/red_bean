#pragma once

#include "global.hpp"
#include <array>
#include <cstddef>
#include <vector>


class Status 
{
public:
    std::array<Bitboard, kColorCount> blocking_squares_{};
    std::array<Bitboard, kColorCount> checking_squares_{};
    
    Bitboard hash_{};
    
    uint16_t full_move_{};
    uint16_t half_move_{};    
    
    Piece captured_piece_{Piece::kNone};
    Square enpassant_{Square::kNoSquare};
    CastlePerm castling_{CastlePerm::kNone};
    Color cur_color_{Color::kWhite};
    bool is_check{};
};

class Board
{
private:

    std::array<Piece, kSquareCount> piece_list_{};      // For each square we can keep track exactly which piece is there.
    std::array<Bitboard, kPieceCount> piece_bb_{};      // Bitboard for each piece
    std::array<Bitboard, kColorCount + 1> color_bb_{};  // Bitboard for each occupancy, + 1 for both sides, reduntand but will save lots of runtime calls later.
    
    std::vector<Status> status_history_;
    uint16_t status_count_;

public:
    Board();

    [[nodiscard]] Bitboard GetPieceBB(const Piece piece) const
    {
        return piece_bb_.at(static_cast<size_t>(piece));
    }

    [[nodiscard]] Bitboard GetColorBB(const Color color) const
    {
        return color_bb_.at(static_cast<size_t>(color));
    }

    [[nodiscard]] Bitboard GetColoredPieceBB(const Color color, const Piece piece) const
    {
        return GetColorBB(color) & GetPieceBB(piece);
    }

    [[nodiscard]] Piece GetPieceOnSquare(const Square sqr) const
    {
        return piece_list_.at(ToIndex(sqr));
    }

    [[nodiscard]] Status& GetCurStatus()
    {
        return status_history_.at(status_count_);
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