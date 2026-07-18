#include "board.hpp"
#include "debug.hpp"
#include "global.hpp"
#include <cassert>
#include <cctype>
#include <cstddef>
#include <exception>
#include <iostream>
#include <string>
#include <sstream>


//Starting position board.
Board::Board() : status_count_(0)
{
    status_history_.reserve(256);
    ParseFenString(kStartingFen);
}



namespace {

void ParseSideToMove(Status& status, const std::string& field)
{
    status.cur_color_ = (field == "w") ? Color::kWhite : Color::kBlack;
}

void ParseCastlingRights(Status& status, const std::string& field)
{
    for (char chr : field)   // field is "-" or some subset of "KQkq"
    {
        switch (chr)
        {
            case 'K':
                status.castling_ = static_cast<CastlePerm>(
                    static_cast<int>(status.castling_) | static_cast<int>(CastlePerm::kWhiteKing));
                break;
            case 'Q':
                status.castling_ = static_cast<CastlePerm>(
                    static_cast<int>(status.castling_) | static_cast<int>(CastlePerm::kWhiteQueen));
                break;
            case 'k':
                status.castling_ = static_cast<CastlePerm>(
                    static_cast<int>(status.castling_) | static_cast<int>(CastlePerm::kBlackKing));
                break;
            case 'q':
                status.castling_ = static_cast<CastlePerm>(
                    static_cast<int>(status.castling_) | static_cast<int>(CastlePerm::kBlackQueen));
                break;
            default:   // '-' falls here
                break;
        }
    }
}

void ParseEnPassant(Status& status, const std::string& field)
{
    if (field == "-") { return; }
    int col = field[0] - 'a';   // 'a'..'h' -> 0..7
    int row = field[1] - '1';   // '1'..'8' -> 0..7  (note: '1', not '0')
    status.enpassant_ = static_cast<Square>(col + (row * 8));
}

void ParseHalfMove(Status& status, const std::string& field)
{
    if (field == "-" || field.empty()) { return; }
    status.half_move_ = static_cast<decltype(status.half_move_)>(std::stoi(field));
}

void ParseFullMove(Status& status, const std::string& field)
{
    if (field == "-" || field.empty()) { return; }
    status.full_move_ = static_cast<decltype(status.full_move_)>(std::stoi(field));
}

}  // namespace

void Board::ParseFenString(const std::string& fen_string)
{ 
    Status temp_status;

    status_history_.clear();
    status_count_ = 0;

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

    std::istringstream iss(fen_string);
    std::string placement;
    std::string side;
    std::string castling;
    std::string enpassant;
    std::string halfmove;
    std::string fullmove;

    iss >> placement >> side >> castling >> enpassant >> halfmove >> fullmove;

    ParseSideToMove(temp_status, side);
    ParseCastlingRights(temp_status, castling);
    ParseEnPassant(temp_status, enpassant);
    ParseHalfMove(temp_status, halfmove);
    ParseFullMove(temp_status, fullmove);

    status_history_.emplace_back(temp_status);
}
