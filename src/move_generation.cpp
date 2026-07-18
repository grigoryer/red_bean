#include "move_generation.hpp"
#include "attacks.hpp"
#include "debug.hpp"
#include "global.hpp"

namespace generation::legal
{

bool IsMoveLegal(Board& board, Move move, Bitboard blockers)
{
    const Status& status = board.GetCurStatus();
    const Color ally_color = status.cur_color_;
    const Bitboard checkers = status.checking_squares_.at(static_cast<size_t>(ally_color));

    const bool is_blocker = (ToBitboard(move.GetFrom()) & blockers) != 0;

    if (is_blocker) 
    { 
        return (ToBitboard(move.GetTo()) & between::GetRayThroughSquares(GetLsb(board.GetColoredPieceBB(ally_color, Piece::kKing)), move.GetFrom())) != 0; 
    }

    if (board.GetPieceOnSquare(move.GetFrom()) == Piece::kKing) // king, need to check if it walks into check during regular and castle
    {
        auto IsSquareAttacked = [&](Square sqr)
        {   
            Bitboard attackers{};
            Color enemy_color = GetEnemyColor(ally_color);

            attackers |= board.GetColoredPieceBB(enemy_color, Piece::kQueen) & attacks::GetQueenAttack(board.GetColorBB(Color::kAll), sqr);
            attackers |= board.GetColoredPieceBB(enemy_color, Piece::kRook) & attacks::GetRookAttack(board.GetColorBB(Color::kAll), sqr);
            attackers |= board.GetColoredPieceBB(enemy_color, Piece::kBishop) & attacks::GetBishopAttack(board.GetColorBB(Color::kAll), sqr);
            attackers |= board.GetColoredPieceBB(enemy_color, Piece::kKnight) & attacks::GetKnightAttack(sqr);
            attackers |= board.GetColoredPieceBB(enemy_color, Piece::kKing) & attacks::GetKingAttack(sqr);
            attackers |= board.GetColoredPieceBB(ally_color, Piece::kPawn) & attacks::GetPawnAttack(enemy_color, sqr);
            
            return (attackers != 0);
        };

    
        if (IsSquareAttacked(move.GetTo()))
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
                && !IsSquareAttacked(transit);
        }
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
