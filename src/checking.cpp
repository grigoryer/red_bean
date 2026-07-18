#include "checking.hpp"
#include "attacks.hpp"
#include "board.hpp"
#include "debug.hpp"
#include "global.hpp"
#include <array>


//Square ksqr = GetLsb(board.GetColoredPieceBB(state.cur_color_, Piece::kKing));

namespace between
{


namespace 
{


    enum class Alignment : uint8_t
    {
        kNone,
        kStraight,
        kDiagonal
    };

    inline Alignment GetAlignment(Square sqr_1, Square sqr_2)
    {
        if (sqr_1 == sqr_2) { return Alignment::kNone; }

        int col_diff = (static_cast<int>(sqr_1) & 7) - (static_cast<int>(sqr_2) & 7);     // col difference
        int row_diff = (static_cast<int>(sqr_1) >> 3) - (static_cast<int>(sqr_2) >> 3);     // row difference

        // Straight line: same col or row
        if (col_diff == 0 || row_diff == 0) { return Alignment::kStraight; }

        // Diagonal line: |df| == |dr| without abs()
        if (std::abs(col_diff) == std::abs(row_diff)) { return Alignment::kDiagonal; }

        return Alignment::kNone;
    }


    

Bitboard GenerateBetween(Square sqr_1, Square sqr_2)
{
    if (GetAlignment(sqr_1, sqr_2) == Alignment::kNone) { return 0; }

    int row_1 = static_cast<int>(sqr_1) >> 3;      // row of sqr_1
    int row_2 = static_cast<int>(sqr_2) >> 3;      // row of sqr_2
    int col_1 = static_cast<int>(sqr_1) & 7;       // col of sqr_1
    int col_2 = static_cast<int>(sqr_2) & 7;       // col of sqr_2

    int row_dir = (row_2 > row_1) ? 1 : (row_2 < row_1) ? -1 : 0;
    int col_dir = (col_2 > col_1) ? 1 : (col_2 < col_1) ? -1 : 0;

    Bitboard line = 0;
    int cur_row = row_1 + row_dir;
    int cur_col = col_1 + col_dir;

    while (cur_row != row_2 || cur_col != col_2)
    {
        int sqr = (cur_row * 8) + cur_col;
        line |= (1ULL << sqr);
        cur_row += row_dir;
        cur_col += col_dir;
    }
    return line;
}

std::array<std::array<Bitboard, kSquareCount>, kSquareCount> intialize_between_ray_array()
{
    std::array<std::array<Bitboard, kSquareCount>, kSquareCount> between_ray_array{};

    for (size_t sqr_1{}; sqr_1 < kSquareCount; ++sqr_1)
    {
        for (size_t sqr_2{}; sqr_2 < kSquareCount; ++sqr_2)
        {
            between_ray_array.at(sqr_1).at(sqr_2) = GenerateBetween(static_cast<Square>(sqr_1), static_cast<Square>(sqr_2));
        }
    }
    return between_ray_array;
}


const std::array<std::array<Bitboard, kSquareCount>, kSquareCount> between_ray_array = intialize_between_ray_array(); 

inline void SetDiagonalCheckers(Board& board, Status& status, Square ksqr, Color color)
{
    // We need to create a ray between king and enemy slider.
    // Get the diagonal sliders and see if the are on the same line.
    Color enemy_color = GetEnemyColor(color);
    Bitboard occ = board.GetColorBB(Color::kAll);
    Bitboard enemy_bishops = board.GetColoredPieceBB(enemy_color, Piece::kBishop) | board.GetColoredPieceBB(enemy_color, Piece::kQueen);

    // Go through all bishops 
    while (enemy_bishops > 0)
    {
        Square enemy_sqr = PopLsb(enemy_bishops);
        Bitboard enemy_attack = attacks::GetBishopAttack(occ, enemy_sqr);

        if (GetAlignment(ksqr, enemy_sqr) != Alignment::kDiagonal)
        {
            continue;
        }
        // If the piece is checking it cannot be blocked no need to waste time on the blocker.
        if ((enemy_attack & ToBitboard(ksqr)) != 0) 
        {
            status.checking_squares_.at(static_cast<int>(color)) |= ToBitboard(enemy_sqr);
            continue;
        }

        // Add blocker
        Bitboard blockers = between_ray_array.at(ToIndex(ksqr)).at(ToIndex(enemy_sqr)) & occ;

        // if total pieces on line is 1 and that piece is ally color, add it to blockers.
        if (BitCount(blockers) == 1 && ((blockers & board.GetColorBB(color)) != 0))
        {
            status.blocking_squares_.at(static_cast<int>(color)) |= blockers;
        }
    }
}


inline void SetStraightCheckers(Board& board, Status& status, Square ksqr, Color color)
{
    // We need to create a ray between king and enemy slider.
    // Get the diagonal sliders and see if the are on the same line.
    Color enemy_color = GetEnemyColor(color);
    Bitboard occ = board.GetColorBB(Color::kAll);
    Bitboard enemy_rooks = board.GetColoredPieceBB(enemy_color, Piece::kRook) | board.GetColoredPieceBB(enemy_color, Piece::kQueen);

    // Go through all bishops 
    while (enemy_rooks > 0)
    {
        Square enemy_sqr = PopLsb(enemy_rooks);
        Bitboard enemy_attack = attacks::GetRookAttack(occ, enemy_sqr);

        if (GetAlignment(ksqr, enemy_sqr) != Alignment::kStraight)
        {
            continue;
        }
        // If the piece is checking it cannot be blocked no need to waste time on the blocker.
        if ((enemy_attack & ToBitboard(ksqr)) != 0) 
        {
            status.checking_squares_.at(static_cast<int>(color)) |= ToBitboard(enemy_sqr);
            continue;
        }

        // Add blocker
        Bitboard blockers = between_ray_array.at(ToIndex(ksqr)).at(ToIndex(enemy_sqr)) & occ;

        // if total pieces on line is 1 and that piece is ally color, add it to blockers.
        if (BitCount(blockers) == 1 && ((blockers & board.GetColorBB(color)) != 0))
        {
            status.blocking_squares_.at(static_cast<int>(color)) |= blockers;
        }
    }
}
} //namespace



void SetSliderCheckersAndBlockers(Board& board, Status& status)
{
    Color ally_color = status.cur_color_;
    Square ksqr = GetLsb(board.GetColoredPieceBB(ally_color, Piece::kKing));
    SetDiagonalCheckers(board, status, ksqr, ally_color);
    SetStraightCheckers(board, status, ksqr, ally_color);
}

Bitboard GetRayBetweenSquares(Square sqr_1, Square sqr_2)
{
    return between_ray_array.at(ToIndex(sqr_1)).at(ToIndex(sqr_2));
}

Bitboard GetRayThroughSquares(Square sqr_1, Square sqr_2)
{
    if (GetAlignment(sqr_1, sqr_2) == Alignment::kNone) { return 0; }

    const int row_1 = static_cast<int>(sqr_1) >> 3;
    const int col_1 = static_cast<int>(sqr_1) & 7;
    const int row_2 = static_cast<int>(sqr_2) >> 3;
    const int col_2 = static_cast<int>(sqr_2) & 7;

    const int row_dir = (row_2 > row_1) - (row_2 < row_1);   // -1, 0, or +1
    const int col_dir = (col_2 > col_1) - (col_2 < col_1);

    // Steps until this axis hits the board edge; a non-moving axis never constrains (8 > any real distance).
    auto steps_to_edge = [](int pos, int dir) { return (dir > 0) ? (7 - pos) : (dir < 0) ? pos : 8; };
    const int steps = std::min(steps_to_edge(row_2, row_dir), steps_to_edge(col_2, col_dir));

    const auto edge = static_cast<Square>(static_cast<int>(sqr_2) + (steps * ((row_dir * 8) + col_dir)));

    return GetRayBetweenSquares(sqr_1, edge) | ToBitboard(edge);
}

}  // namespace between