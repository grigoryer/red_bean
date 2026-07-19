#include "move_generation.hpp"
#include "attacks.hpp"
#include "global.hpp"
#include <cassert>

namespace generation::legal
{

bool IsMoveLegal(Board& board, Move move, Bitboard blockers)
{
    const Status& status = board.GetCurStatus();
    const Color ally_color = status.cur_color_;
    const Color enemy_color = GetEnemyColor(ally_color);
    const bool is_blocker = (ToBitboard(move.GetFrom()) & blockers) != 0;

    auto IsSquareAttacked = [&](Square sqr, Bitboard occ)
    {   
        Bitboard attackers{};

        attackers |= board.GetColoredPieceBB(enemy_color, Piece::kQueen) & attacks::GetQueenAttack(occ, sqr);
        attackers |= board.GetColoredPieceBB(enemy_color, Piece::kRook) & attacks::GetRookAttack(occ, sqr);
        attackers |= board.GetColoredPieceBB(enemy_color, Piece::kBishop) & attacks::GetBishopAttack(occ, sqr);
        attackers |= board.GetColoredPieceBB(enemy_color, Piece::kKnight) & attacks::GetKnightAttack(sqr);
        attackers |= board.GetColoredPieceBB(enemy_color, Piece::kKing) & attacks::GetKingAttack(sqr);
        attackers |= board.GetColoredPieceBB(enemy_color, Piece::kPawn) & attacks::GetPawnAttack(ally_color, sqr);
        
        return (attackers & occ) != 0;
    };

    //PrintMove(move);
    if (is_blocker) 
    { 
        return (ToBitboard(move.GetTo()) & between::GetRayThroughSquares(GetLsb(board.GetColoredPieceBB(ally_color, Piece::kKing)), move.GetFrom())) != 0; 
    }

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
            return (board.GetCurStatus().checking_squares_.at(static_cast<size_t>(ally_color)) == 0)
                && !IsSquareAttacked(transit, kingless_occupancy);
        }
    }   

    if (move.IsEnPassant())
    {
        auto captured_square{static_cast<Square>(static_cast<int>(move.GetTo()) + (ally_color == Color::kWhite ? -8 : 8))};
        Bitboard occ = (board.GetColorBB(Color::kAll) & ~ToBitboard(move.GetFrom()) & ~ToBitboard(captured_square)) | ToBitboard(move.GetTo());;

        return !IsSquareAttacked(GetLsb(board.GetColoredPieceBB(ally_color, Piece::kKing)), occ);
    }

    return true;
};

void CleanMoveList(Board& board, MoveList& move_list)
{
    size_t legal_count{};
    const Color ally_color = board.GetCurStatus().cur_color_;
    const Bitboard& blockers = board.GetCurStatus().blocking_squares_.at(static_cast<size_t>(ally_color));

    for (size_t ind = 0; ind < move_list.move_count; ++ind)
    {
        const Move move = move_list.list.at(ind);
        if (IsMoveLegal(board, move, blockers))
        {
            move_list.list.at(legal_count) = move;
            ++legal_count;
        }
    }
    move_list.move_count = legal_count;
}

}  // namespace generation::legal
