#pragma once

#include "board.hpp"
#include "global.hpp"
#include <array>
#include <cstddef>
#include <iostream>

static const std::array<char, (static_cast<size_t>(kPieceCount * 2))> kPieceChars{'Q', 'R', 'B', 'N', 'P', 'K',
                                                                                  'q', 'r', 'b', 'n', 'p', 'k' };

inline void PrintBitboard(const Bitboard& bb)
{
    std::cout << "\n";
    for (size_t row{kRowCount}; row > 0; --row)
    {
        std::cout << "\n" << row << "  ";
        for (size_t col{0}; col < kColumnCount ; ++col)
        {
            auto sqr = static_cast<Square>(((col) + ((row - 1) * 8)));
            std::cout << (GetBit(bb, sqr) ? "1" : ".") << " ";
        }
    }

    std::cout << "\n   A B C D E F G H\n";
}


inline void PrintPieceBoard(const Board& board)
{
    std::cout << "\n";
    for (size_t row{kRowCount}; row > 0; --row)
    {
        std::cout << "\n" << row << "  ";
        for (size_t col{0}; col < kColumnCount ; ++col)
        {
            auto sqr = static_cast<Square>(((col) + ((row - 1) * 8)));
            auto piece = static_cast<int>(board.GetPieceOnSquare(sqr));

            if (static_cast<Piece>(piece) == Piece::kNone) { std::cout << ". "; continue; }

            auto is_black = static_cast<bool>(board.GetColorBB(Color::kBlack) & ToBitboard(sqr));
            size_t char_index = piece + (is_black ? 6 : 0);
            std::cout << kPieceChars.at(char_index) << ' ';
        }
    }

    std::cout << "\n   A B C D E F G H\n";
}

