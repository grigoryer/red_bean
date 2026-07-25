#include "move_generation.hpp"
#include "search_globals.hpp"
#include "attacks.hpp"
#include "global.hpp"

#include <cassert>

namespace generation::legal
{

bool IsPseudoLegal(Board& board, const ExtMove move)
{
    const Status& status = board.GetCurStatus();
    const Color ally_color = status.cur_color_;
    const Color enemy_color = GetEnemyColor(ally_color);
 
    const Square from_sqr = move.GetFrom();
    const Square to_sqr = move.GetTo();
 
    if (from_sqr == to_sqr) { return false; }
 
    const Bitboard from_bb = ToBitboard(from_sqr);
    const Bitboard to_bb = ToBitboard(to_sqr);
    const Bitboard occ = board.GetColorBB(Color::kAll);
 
    // Must move one of our own pieces, and must not land on our own piece.
    // This also covers "from_sqr is empty" without trusting piece_list_.
    if ((from_bb & board.GetColorBB(ally_color)) == 0) { return false; }
    if ((to_bb & board.GetColorBB(ally_color)) != 0) { return false; }
 
    const Piece piece = board.GetPieceOnSquare(from_sqr);
    const MoveType move_type = move.GetType();
 
    // Castling legality check
    if (move_type == MoveType::kCastle)
    {
        if (piece != Piece::kKing) { return false; }
 
        const bool is_white = (ally_color == Color::kWhite);
        if (from_sqr != (is_white ? Square::e1 : Square::e8)) { return false; }
 
        const Column to_col = GetSquareCol(to_sqr);
        if (to_col != Column::C && to_col != Column::G) { return false; }
        if (GetSquareRow(to_sqr) != GetSquareRow(from_sqr)) { return false; }
 
        const bool queen_side = (to_col == Column::C);
        const auto required = static_cast<int>(is_white ? (queen_side ? CastlePerm::kWhiteQueen : CastlePerm::kWhiteKing) : (queen_side ? CastlePerm::kBlackQueen : CastlePerm::kBlackKing));
 
        if ((static_cast<int>(status.castling_) & required) == 0) { return false; }
 
        const Square rook_sqr = is_white ? (queen_side ? Square::a1 : Square::h1) : (queen_side ? Square::a8 : Square::h8);
 
        if ((board.GetColoredPieceBB(ally_color, Piece::kRook) & ToBitboard(rook_sqr)) == 0) { return false; }
        if ((between::GetRayBetweenSquares(from_sqr, rook_sqr) & occ) != 0) { return false; }
 
        return true;  // transit-square safety is IsMoveLegal's job
    }
 
    // En Passant legality check
    if (move_type == MoveType::kEnPassant)
    {
        if (piece != Piece::kPawn) { return false; }
        if (status.enpassant_ == Square::kNoSquare || to_sqr != status.enpassant_) { return false; }
        if ((occ & to_bb) != 0) { return false; }
 
        // from_sqr must be a square an ally pawn attacks the ep square from.
        if ((attacks::GetPawnAttack(enemy_color, to_sqr) & from_bb) == 0) { return false; }
 
        const auto victim_sqr = static_cast<Square>(static_cast<int>(to_sqr) + (ally_color == Color::kWhite ? -8 : 8));
 
        return (board.GetColoredPieceBB(enemy_color, Piece::kPawn) & ToBitboard(victim_sqr)) != 0;
    }
 
    // No promo-piece validation needed: the 2-bit field offset from kQueen can
    // only decode to Q/R/B/N, so GetPromo() is safe by construction.
 
    // ---- Pawns (normal + promotion share the same geometry) -----------------
    if (piece == Piece::kPawn)
    {
        const Bitboard promo_rank = ToBitboard(ally_color == Color::kWhite ? Row::_7 : Row::_2);
        const bool on_promo_rank = (from_bb & promo_rank) != 0;
 
        // A pawn on the pre-promotion rank must promote, and one anywhere else must not.
        if (on_promo_rank != (move_type == MoveType::kPromotion)) { return false; }
 
        const int push = (ally_color == Color::kWhite) ? 8 : -8;
        const int delta = static_cast<int>(to_sqr) - static_cast<int>(from_sqr);
 
        if ((attacks::GetPawnAttack(ally_color, from_sqr) & to_bb) != 0)
        {
            // Diagonal is only legal as a real capture (ep already handled above).
            return (to_bb & board.GetColorBB(enemy_color)) != 0;
        }
 
        if (delta == push)
        {
            return (to_bb & occ) == 0;
        }
 
        if (delta == 2 * push)
        {
            const Bitboard start_rank = ToBitboard(ally_color == Color::kWhite ? Row::_2 : Row::_7);
            const auto mid_sqr = static_cast<Square>(static_cast<int>(from_sqr) + push);
            return (from_bb & start_rank) != 0 && (occ & (to_bb | ToBitboard(mid_sqr))) == 0;
        }
 
        return false;
    }
 
    if (move_type != MoveType::kNormal) { return false; }
 
    switch (piece)
    {
        case Piece::kKnight: return (attacks::GetKnightAttack(from_sqr) & to_bb) != 0;
        case Piece::kKing: return (attacks::GetKingAttack(from_sqr) & to_bb) != 0;
        case Piece::kBishop: return (attacks::GetBishopAttack(occ, from_sqr) & to_bb) != 0;
        case Piece::kRook: return (attacks::GetRookAttack(occ, from_sqr) & to_bb) != 0;
        case Piece::kQueen: return ((attacks::GetBishopAttack(occ, from_sqr) | attacks::GetRookAttack(occ, from_sqr)) & to_bb) != 0;
        default: return false;
    }
}

inline bool IsMoveLegal(Board& board, const Status& status, const ExtMove move, const Bitboard blockers, const Color ally_color, const Color enemy_color)
{
    if ((ToBitboard(move.GetFrom()) & blockers) != 0) 
    { 
        return (ToBitboard(move.GetTo()) & between::GetRayThroughSquares(GetLsb(board.GetColoredPieceBB(ally_color, Piece::kKing)), move.GetFrom())) != 0; 
    }
    
    // place here for edge case of double push, we need to remove the attackign pawn in case of en passant captured by ref; EP narrows this in place
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

bool IsStoredMoveLegal(Board& board, const ExtMove move)
{
    const Status& status = board.GetCurStatus();
    if (!IsPseudoLegal(board, move)) { return false; }

    const Color ally_color = status.cur_color_;
    const Color enemy_color = GetEnemyColor(ally_color);

    const Bitboard slider_checkers = status.checking_squares_;
    const Bitboard leaper_checkers = status.leaper_checking_squares_;
    const size_t checker_count = BitCount(slider_checkers) + BitCount(leaper_checkers);

    const Square king_sqr = GetLsb(board.GetColoredPieceBB(ally_color, Piece::kKing));

    if (move.GetFrom() != king_sqr)
    {
        if (checker_count > 1) { return false; }

        if (checker_count == 1)
        {
            Bitboard target = leaper_checkers;
            if (slider_checkers != 0)
            {
                target = between::GetRayBetweenSquares(king_sqr, GetLsb(slider_checkers)) | slider_checkers;
            }

            Bitboard landing = ToBitboard(move.GetTo());
            if (move.IsEnPassant())
            {
                landing |= ToBitboard(static_cast<Square>(
                    static_cast<int>(move.GetTo()) + (ally_color == Color::kWhite ? -8 : 8)));
            }

            if ((landing & target) == 0) { return false; }
        }
    }
    else if (move.IsCastle() && checker_count != 0)
    {
        return false;
    }

    return IsMoveLegal(board, status, move, status.blocking_squares_, ally_color, enemy_color);
}

void CleanMoveList(Board& board, MoveList& move_list)
{
    size_t legal_count{};
    const Status& status = board.GetCurStatus();
    const Color ally_color = status.cur_color_;
    const Color enemy_color = GetEnemyColor(ally_color);

    const Bitboard& blockers = board.GetCurStatus().blocking_squares_;

    for (size_t ind = 0; ind < move_list.move_count; ++ind)
    {
        const ExtMove move = move_list.list.at(ind);
        // Also add scoring to the moves in here.
        if (IsMoveLegal(board, status, move, blockers, ally_color, enemy_color))
        {
            move_list.list[legal_count] = move;
            legal_count++;
        }

    }
    move_list.move_count = legal_count;
}

}  // namespace generation::legal
