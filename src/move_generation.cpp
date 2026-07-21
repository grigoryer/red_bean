#include "move_generation.hpp"
#include "attacks.hpp"
#include "global.hpp"
#include <cassert>

namespace generation::legal
{


inline bool IsMoveLegal(Board& board, const Status& status, const Move move, const Bitboard blockers, const Color ally_color, const Color enemy_color)
{
    if ((ToBitboard(move.GetFrom()) & blockers) != 0) 
    { 
        return (ToBitboard(move.GetTo()) & between::GetRayThroughSquares(GetLsb(board.GetColoredPieceBB(ally_color, Piece::kKing)), move.GetFrom())) != 0; 
    }
    
    // place here for edge case of double push, we need to remove the attackign pawn in case of en passant.
    // captured by ref; EP narrows this in place
    Bitboard enemy_pawns = board.GetColoredPieceBB(enemy_color, Piece::kPawn); 
    
    auto IsSquareAttacked = [&](Square sqr, Bitboard occ)
    {   

        if (board.GetColoredPieceBB(enemy_color, Piece::kKnight) & attacks::GetKnightAttack(sqr)) { return true; } 
        if (board.GetColoredPieceBB(enemy_color, Piece::kKing) & attacks::GetKingAttack(sqr)) { return true; }
        if (enemy_pawns & attacks::GetPawnAttack(ally_color, sqr)) { return true; }

        Bitboard queens = board.GetColoredPieceBB(enemy_color, Piece::kQueen);
        if ((board.GetColoredPieceBB(enemy_color, Piece::kRook) | queens) & attacks::GetRookAttack(occ, sqr)) { return true; }
        if ((board.GetColoredPieceBB(enemy_color, Piece::kBishop) | queens) & attacks::GetBishopAttack(occ, sqr)) { return true; }
        
        return false;
    };


    if (board.GetPieceOnSquare(move.GetFrom()) == Piece::kKing) // king, need to check if it walks into check during regular and castle
    {
        Bitboard kingless_occupancy = board.GetColorBB(Color::kAll) & ~ToBitboard(move.GetFrom());
        if (IsSquareAttacked(move.GetTo(), kingless_occupancy))
        {
            return false;
        }

        //checks if the piece that the King jumps on are under attack. We already check the distanation above 
        //so only need to check the intermitened jump
        if (move.IsCastle())
        {   
            // if get to column is c thats queen side and if it is g its king side. 
            // if queen side we need to check square + 1 and king side - 1;
            int castle_dir = (GetSquareCol(move.GetTo()) == Column::C ? 1 : -1);
            auto transit = static_cast<Square>(static_cast<int>(move.GetTo()) + castle_dir);
            return (status.checking_squares_ == 0)
                && !IsSquareAttacked(transit, kingless_occupancy);
        }
    }   

    if (move.IsEnPassant())
    {   
        auto captured_square{static_cast<Square>(static_cast<int>(move.GetTo()) + (ally_color == Color::kWhite ? -8 : 8))};

        Bitboard occ = (board.GetColorBB(Color::kAll) & ~ToBitboard(move.GetFrom()) & ~ToBitboard(captured_square)) | ToBitboard(move.GetTo());
        enemy_pawns &= ~ToBitboard(captured_square);

        return !IsSquareAttacked(GetLsb(board.GetColoredPieceBB(ally_color, Piece::kKing)), occ);
    }

    return true;
};

void CleanMoveList(Board& board, MoveList& move_list)
{
    size_t legal_count{};
    const Status& status = board.GetCurStatus();
    const Color ally_color = status.cur_color_;
    const Color enemy_color = GetEnemyColor(ally_color);

    const Bitboard& blockers = board.GetCurStatus().blocking_squares_;

    for (size_t ind = 0; ind < move_list.move_count; ++ind)
    {
        const Move move = move_list.list.at(ind);
        if (IsMoveLegal(board, status, move, blockers, ally_color, enemy_color))
        {
            move_list.list[legal_count++] = move;
        }
    }
    move_list.move_count = legal_count;
}

}  // namespace generation::legal
