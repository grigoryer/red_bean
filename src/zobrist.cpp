#include "zobrist.hpp"
#include "board.hpp"
#include "global.hpp"
#include <cassert>
#include <cstddef>


namespace Zobrist
{

void InitailizBoardHash(Board &board)
{
    Hash new_hash{};
    auto& status = board.GetCurStatus();
    new_hash ^= ZobCastling(status.castling_);
    new_hash ^= ZobColor(status.cur_color_);
    new_hash ^= ZobEnpassant(status.enpassant_);
    
    for (size_t sqr_i{}; sqr_i < kSquareCount; ++sqr_i)
    {
        const auto square = static_cast<Square>(sqr_i);
        const auto piece = board.GetPieceOnSquare(square);
        if (piece == Piece::kNone)
        {
            continue;
        }

        const Color color = (board.GetColorBB(Color::kWhite) & ToBitboard(square)) != 0
                                ? Color::kWhite
                                : Color::kBlack;
        new_hash ^= ZobPiece(piece, square, color);
    }

    status.hash_ = new_hash;
}

}  // namespace Zobrist