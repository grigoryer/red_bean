#pragma once

#include "global.hpp"
#include "board.hpp"


namespace Zobrist
{
    Hash ZobEnpassant(Square ep_sqr);   
    Hash ZobPiece(Piece piece, Square sqr, Color color);
    Hash ZobCastling(CastlePerm castling);
    Hash ZobColor(Color color);
    void InitailizBoardHash(Board &board);
}