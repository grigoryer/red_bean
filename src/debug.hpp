#pragma once

#include "board.hpp"
#include "global.hpp"
#include "move.hpp"
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


inline void PrintMove(Move move)
{
    std::cout << SquareToString(move.GetFrom()) << " "
                << SquareToString(move.GetTo()) << " ";

    std::string type{};
    switch (move.GetType())
    {
        case MoveType::kNormal: type = "Normal"; break; 
        case MoveType::kPromotion: type = "Promo"; break; 
        case MoveType::kEnPassant: type = "EnPassant"; break; 
        case MoveType::kCastle: type = "Castle"; break; 
        break;
    }

    std::cout << type;
    if (move.GetType() == MoveType::kNormal)
    {
        std::cout << "\n";
        return;
    }

    std::string promo{};
    if (move.IsPromotion())
    {
        switch (move.GetPromo()) 
        {
            case Piece::kQueen: promo = " Queen"; break; 
            case Piece::kRook: promo = " Rook"; break; 
            case Piece::kBishop: promo = " Bishop"; break; 
            case Piece::kKnight: promo = " Knight"; break; 
        }
    }
    std::cout << promo;

    std::string castle{};
    if (move.IsCastle())
    {
        switch (move.GetTo())
        {
            case Square::g1: case Square::g8: castle = " King Side"; break;
            case Square::c1: case Square::c8: castle = " Queen Side"; break;
            default: break;
        }
    }
    std::cout << castle;


    std::cout << "\n";
}


inline void PrintMoveList(const MoveList& move_list)
{
    std::cout << "Move List (" << move_list.move_count << "):\n";
    for (size_t i = 0; i < move_list.move_count; ++i)
    {
        std::cout << i + 1 << ": ";
        PrintMove(move_list.list.at(i));
    }
}

inline void PrintStatus(const Status& status)
{
    std::cout << "=== Status ===\n";

    std::cout << "Side to move : "
              << (status.cur_color_ == Color::kWhite ? "White" : "Black") << "\n";

    // Castling rights as KQkq (dash if none)
    std::cout << "Castling     : ";
    const int c = static_cast<int>(status.castling_);
    std::string rights;
    if (c & static_cast<int>(CastlePerm::kWhiteKing))  { rights += 'K'; }
    if (c & static_cast<int>(CastlePerm::kWhiteQueen)) { rights += 'Q'; }
    if (c & static_cast<int>(CastlePerm::kBlackKing))  { rights += 'k'; }
    if (c & static_cast<int>(CastlePerm::kBlackQueen)) { rights += 'q'; }
    std::cout << (rights.empty() ? "-" : rights) << "\n";

    // En passant square, or "-"
    std::cout << "En passant   : "
              << (status.enpassant_ == Square::kNoSquare
                      ? "-"
                      : SquareToString(status.enpassant_)) << "\n";

    // Captured piece (only meaningful after a capture make-move)
    std::cout << "Captured     : "
              << (status.captured_piece_ == Piece::kNone
                      ? 'X'
                      : kPieceChars.at(static_cast<int>(status.captured_piece_))) << "\n";

    std::cout << "Half move    : " << static_cast<int>(status.half_move_) << "\n";
    std::cout << "Full move    : " << static_cast<int>(status.full_move_) << "\n";
    std::cout << "==============\n";
}