#include "board.hpp"
#include "debug.hpp"
#include "global.hpp"
#include <cassert>
#include <cctype>
#include <cstddef>
#include <exception>
#include <iostream>
#include <string>



void Board::ParseFenString(const std::string& fen_string)
{ 
    for (auto& bb : piece_bb_) { bb = 0ULL; }
    for (auto& bb : color_bb_) { bb = 0ULL; }

    piece_list_.fill(Piece::kNone);

    auto itr = fen_string.begin();
    auto cur_sqr = static_cast<int>(Square::a8); // keep at size_t than cast to square when inserting.

    while (*itr != ' ')
    {
        char chr = *itr;

        if (std::isdigit(static_cast<unsigned char>(chr)) == 0 && chr != '/')   // it's a piece letter
        { 
            bool is_white = std::isupper(static_cast<unsigned char>(chr)) != 0;
            chr = static_cast<char>(std::tolower(static_cast<unsigned char>(chr)));
            SetColorBB(static_cast<Color>(!is_white), static_cast<Square>(cur_sqr)); //flip is white since white is index 0 in enum.
            SetColorBB(Color::kAll, static_cast<Square>(cur_sqr));
        }

        switch(chr)
        {
            case 'k': 
            {
                SetPieceOnSquare(Piece::kKing, static_cast<Square>(cur_sqr));
                SetPieceBB(Piece::kKing, static_cast<Square>(cur_sqr++)); break; 
            }
            case 'q': 
            {
                SetPieceOnSquare(Piece::kQueen, static_cast<Square>(cur_sqr));
                SetPieceBB(Piece::kQueen, static_cast<Square>(cur_sqr++)); break; 
            }
            case 'r': 
            {
                SetPieceOnSquare(Piece::kRook, static_cast<Square>(cur_sqr));
                SetPieceBB(Piece::kRook, static_cast<Square>(cur_sqr++)); break; 
            }
            case 'b': 
            {
                SetPieceOnSquare(Piece::kBishop, static_cast<Square>(cur_sqr));
                SetPieceBB(Piece::kBishop, static_cast<Square>(cur_sqr++)); break; 
            }
            case 'n': 
            {
                SetPieceOnSquare(Piece::kKnight, static_cast<Square>(cur_sqr));
                SetPieceBB(Piece::kKnight, static_cast<Square>(cur_sqr++)); break; 
            }
            case 'p': 
            {
                SetPieceOnSquare(Piece::kPawn, static_cast<Square>(cur_sqr));
                SetPieceBB(Piece::kPawn, static_cast<Square>(cur_sqr++)); break; 
            }
            default : 
            {
                if (chr == '/')
                {
                    cur_sqr -= ((kColumnCount * 2)); // Go down two rows.
                    break;
                }

                int skip_num = chr - '0';
                if (skip_num > 0 && skip_num <= kColumnCount)
                {
                    cur_sqr += skip_num;
                    break;
                }

                std::terminate();
            }
        }
        itr++;
    }

    assert(cur_sqr == 8);



}   


namespace
{



} //namespace