#pragma once

#include "global.hpp"
#include <array>
#include <cstddef>
#include <array>


class Status 
{
public:
    Bitboard blocking_squares_{};
    Bitboard checking_squares_{};
    Bitboard leaper_checking_squares_{};
    
    Bitboard hash_{};
    int phase_num{};
    
    uint16_t full_move_{};
    uint16_t half_move_{};    
    
    Piece captured_piece_{Piece::kNone};
    Square enpassant_{Square::kNoSquare};
    CastlePerm castling_{CastlePerm::kNone};
    Color cur_color_{Color::kWhite};
};

class Board
{
private:

    std::array<Piece, kSquareCount> piece_list_{};      // For each square we can keep track exactly which piece is there.
    std::array<Bitboard, kPieceCount> piece_bb_{};      // Bitboard for each piece
    std::array<Bitboard, kColorCount + 1> color_bb_{};  // Bitboard for each occupancy, + 1 for both sides, reduntand but will save lots of runtime calls later.
    
    std::array<Status, 512> status_history_;
    uint16_t status_count_{};

    
public:

    Board();

    explicit Board(const std::string& fen_string);

    [[nodiscard]] Bitboard GetPieceBB(const Piece piece) const
    {
        return piece_bb_[(static_cast<size_t>(piece))];
    }

    [[nodiscard]] Bitboard GetColorBB(const Color color) const
    {
        return color_bb_[(static_cast<size_t>(color))];
    }

    [[nodiscard]] Bitboard GetColoredPieceBB(const Color color, const Piece piece) const
    {
        return GetColorBB(color) & GetPieceBB(piece);
    }

    [[nodiscard]] Piece GetPieceOnSquare(const Square sqr) const
    {
        return piece_list_[(ToIndex(sqr))];
    }

    void MovePieceBB(const Piece piece, const Square from_sqr, const Square to_sqr, const Color color)
    {
        Bitboard mask = ToBitboard(from_sqr) | ToBitboard(to_sqr);
        piece_bb_[static_cast<size_t>(piece)] ^= mask;
        color_bb_[static_cast<size_t>(color)] ^= mask;
        color_bb_[static_cast<size_t>(Color::kAll)] ^= mask;
    }

    void SetPieceBB(const Piece piece, const Square sqr)
    {
        SetBit(piece_bb_[(static_cast<size_t>(piece))], sqr);
    }

    void SetColorBB(const Color color, const Square sqr)
    {
        SetBit(color_bb_[static_cast<size_t>(color)], sqr);
    }

    // Remove item
    void RemovePieceBB(const Piece piece, const Square sqr)
    {
        PopBit(piece_bb_[static_cast<size_t>(piece)], sqr);
    }

    void RemoveColorBB(const Color color, const Square sqr)
    {
        PopBit(color_bb_[static_cast<size_t>(color)], sqr);
    }

    // can be used for removing since piece::kNone is a piece
    void SetPieceOnSquare(const Piece piece, const Square sqr)
    {
        piece_list_[ToIndex(sqr)] = piece;
    }

    [[nodiscard]] Status& GetCurStatus()
    {
        return status_history_[status_count_];
    }

    [[nodiscard]] Status& GetNextStatus()
    {
        return status_history_[status_count_];
    }

    [[nodiscard]] Status& CreateNewStatus()
    {
        status_history_[status_count_ + 1] = status_history_[status_count_];
        return status_history_[++status_count_];
    }

    void PopStatus()
    {
        --status_count_;
    }

    [[nodiscard]] bool IsDraw() const
    {
        const Status& cur = status_history_[status_count_];
        if (cur.half_move_ >= 100) { return true; }

        const int cur_index = static_cast<int>(status_count_);
        const int lower_bound = std::max(0, cur_index - static_cast<int>(cur.half_move_));

        int same_hash_count{};
        for (int i{cur_index - 2}; i >= lower_bound; i -= 2)
        {
            if (status_history_[i].hash_ == cur.hash_)
            {
                if (++same_hash_count >= 2) { return true; }
            }
        }
        return false;
    }

    void ParseFenString(const std::string& fen_string);
    void ResetBoard();
    void ResetBoard(const std::string& fen_string);

};