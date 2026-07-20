#pragma once

#include "global.hpp"
#include "board.hpp"


namespace Zobrist
{
    inline Hash ZobEnpassant(Square ep_sqr);   
    inline Hash ZobPiece(Piece piece, Square sqr, Color color);
    inline Hash ZobCastling(CastlePerm castling);
    inline Hash ZobColor(Color color);
    void InitailizBoardHash(Board &board);
}