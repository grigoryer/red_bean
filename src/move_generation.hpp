#pragma once

#include "attacks.hpp"
#include "checking.hpp"
#include "global.hpp"
#include "move.hpp"
#include "board.hpp"


namespace generation
{

namespace pseudolegal
{


template <Color color>
inline void GeneratePawnQuiet(Board& board, MoveList& move_list, Bitboard target)
{
    Bitboard pawn_spots = board.GetColoredPieceBB(color, Piece::kPawn);

    constexpr Bitboard promo_rank = ToBitboard((color == Color::kWhite) ? Row::_7 : Row::_2);
    constexpr Bitboard double_rank = ToBitboard((color == Color::kWhite) ? Row::_4 : Row::_5);
    constexpr int from_shift = (color == Color::kWhite) ? -8 : 8;

    //Shift depending on color, either up or down. And if it is not occupied at it do the bitboard.
    pawn_spots = pawn_spots & ~promo_rank; // remove the promo rank pawns immediatly th
    Bitboard single_move = ~board.GetColorBB(Color::kAll) & ((color == Color::kWhite) ?  pawn_spots << 8 : pawn_spots >> 8);

    //Shift again for double move but only keep if it is on the double rank and not occupied.
    Bitboard double_move = ~board.GetColorBB(Color::kAll) & double_rank & ((color == Color::kWhite) ? single_move << 8 : single_move >> 8);

    single_move &= target;
    double_move &= target;

    while (single_move != 0)
    {
        Square to_sqr = PopLsb(single_move);
        auto from_sqr = static_cast<Square>(static_cast<int>(to_sqr) + from_shift);

        move_list.addMove(Move(from_sqr, to_sqr, MoveType::kNormal));
    }

    while (double_move != 0)
    {
        Square to_sqr = PopLsb(double_move);
        auto from_sqr = static_cast<Square>(static_cast<int>(to_sqr) + from_shift + from_shift);

        move_list.addMove(Move(from_sqr, to_sqr, MoveType::kNormal));
    }
}

// captures and en passants
template <Color color>
inline void GeneratePawnNoisy(Board& board, MoveList& move_list, Bitboard target)
{
    Bitboard pawn_spots = board.GetColoredPieceBB(color, Piece::kPawn);
    Bitboard enemy_targets = board.GetColorBB(GetEnemyColor(color));

    // promotion items. Quiet promotion is in here.
    constexpr Bitboard promo_rank = ToBitboard((color == Color::kWhite) ? Row::_7 : Row::_2);
    constexpr int from_shift = (color == Color::kWhite) ? -8 : 8;
    Bitboard quiet_promo_pawns = (pawn_spots & promo_rank);
    quiet_promo_pawns = ~board.GetColorBB(Color::kAll) & ((color == Color::kWhite) ? quiet_promo_pawns << 8 : quiet_promo_pawns >> 8);

    quiet_promo_pawns &= target;

    while (pawn_spots != 0)
    {
        Square cur_sqr = PopLsb(pawn_spots);
        Bitboard attacks = enemy_targets & attacks::GetPawnAttack(color, cur_sqr) & target;
        while (attacks != 0)
        {
            Square to_sqr = PopLsb(attacks);
            if ((ToBitboard(cur_sqr) & promo_rank) != 0)
            {
                // capture-promotions: one move per promo piece (Q,R,B,N)
                move_list.addMove(Move(cur_sqr, to_sqr, MoveType::kPromotion, Piece::kQueen));
                move_list.addMove(Move(cur_sqr, to_sqr, MoveType::kPromotion, Piece::kRook));
                move_list.addMove(Move(cur_sqr, to_sqr, MoveType::kPromotion, Piece::kBishop));
                move_list.addMove(Move(cur_sqr, to_sqr, MoveType::kPromotion, Piece::kKnight));
            }
            else
            {
                move_list.addMove(Move(cur_sqr, to_sqr, MoveType::kNormal));
            }
        }
    }

    while (quiet_promo_pawns != 0)
    {
        Square to_sqr = PopLsb(quiet_promo_pawns);
        auto from_sqr = static_cast<Square>(static_cast<int>(to_sqr) + from_shift);

        move_list.addMove(Move(from_sqr, to_sqr, MoveType::kPromotion, Piece::kQueen));
        move_list.addMove(Move(from_sqr, to_sqr, MoveType::kPromotion, Piece::kRook));
        move_list.addMove(Move(from_sqr, to_sqr, MoveType::kPromotion, Piece::kBishop));
        move_list.addMove(Move(from_sqr, to_sqr, MoveType::kPromotion, Piece::kKnight));
    }

    // should work without any target
    if (board.GetCurStatus().enpassant_ != Square::kNoSquare)
    {
        Bitboard enpassant_squares = attacks::GetPawnAttack(GetEnemyColor(color), board.GetCurStatus().enpassant_) 
                                            & board.GetColoredPieceBB(color, Piece::kPawn);
        while (enpassant_squares != 0)
        {
            move_list.addMove(Move(PopLsb(enpassant_squares), board.GetCurStatus().enpassant_, MoveType::kEnPassant));
        }
    }
}

template<GenType gen_type, Color color>
void GeneratePawnMoves(Board& board, MoveList& move_list, Bitboard target)
{
    if constexpr (gen_type == GenType::kNoisy || gen_type == GenType::kAll)
    {
        GeneratePawnNoisy<color>(board, move_list, target);
    }
    
    if constexpr (gen_type == GenType::kQuiet || gen_type == GenType::kAll)
    {
        GeneratePawnQuiet<color>(board, move_list, target);
    }
}

template<Piece piece, GenType gen_type>
void GenerateForPiece(Board& board, MoveList& move_list, Bitboard occ, Color ally_color, Color enemy_color, Bitboard target)
{
    Bitboard piece_spots = board.GetColoredPieceBB(ally_color, piece);
    while (piece_spots != 0)
    {
        Square from_sqr = PopLsb(piece_spots);
        Bitboard piece_moves = attacks::getPieceAttack<piece>(occ, from_sqr, ally_color) & ~board.GetColorBB(ally_color);

        if constexpr (gen_type == GenType::kNoisy || gen_type == GenType::kAll)
        {
            Bitboard noisy_moves = piece_moves & board.GetColorBB(enemy_color) & target; // captures
            while (noisy_moves != 0)
            {
                move_list.addMove(Move(from_sqr, PopLsb(noisy_moves), MoveType::kNormal));
            }
        }
        if constexpr (gen_type == GenType::kQuiet || gen_type == GenType::kAll)
        {
            Bitboard quiet_moves = piece_moves & ~board.GetColorBB(enemy_color) & target; // non-captures
            while (quiet_moves != 0)
            {
                move_list.addMove(Move(from_sqr, PopLsb(quiet_moves), MoveType::kNormal));
            }
        }
    }
}

template<GenType gen_type>
void GenerateMajorPieceMoves(Board& board, MoveList& move_list, Bitboard target)
{
    Color ally_color = board.GetCurStatus().cur_color_;
    Color enemy_color = GetEnemyColor(ally_color);
    Bitboard occ = board.GetColorBB(Color::kAll);

    GenerateForPiece<Piece::kQueen, gen_type>(board, move_list, occ, ally_color, enemy_color, target);
    GenerateForPiece<Piece::kRook, gen_type>(board, move_list, occ, ally_color, enemy_color, target);
    GenerateForPiece<Piece::kBishop, gen_type>(board, move_list, occ, ally_color, enemy_color, target);
    GenerateForPiece<Piece::kKnight, gen_type>(board, move_list, occ, ally_color, enemy_color, target);
}

// checks if king is in original posisiton, there are required permissions, and if there is anything blocking rook or king from moving.
inline void GenerateCastling(Board& board, MoveList& move_list)
{
    Color ally_color = board.GetCurStatus().cur_color_;
    const bool is_white = (ally_color == Color::kWhite);

    auto required_perm = (static_cast<int>(is_white ? CastlePerm::kWhiteBoth : CastlePerm::kBlackBoth));
    auto perm = static_cast<int>(board.GetCurStatus().castling_);
    auto rights = perm & required_perm;

    if (rights == static_cast<int>(CastlePerm::kNone)) { return; }

    Square from_sqr = GetLsb(board.GetColoredPieceBB(ally_color, Piece::kKing));
    Square required_king_sqr = is_white ? Square::e1 : Square::e8;
    
    if (from_sqr != required_king_sqr) { return; }

    Square queen_side = (is_white ? Square::a1 : Square::a8);
    Square king_side = (is_white ? Square::h1 : Square::h8);

    Square queen_to = (is_white ? Square::c1 : Square::c8);
    Square king_to = (is_white ? Square::g1 : Square::g8);

    bool queen_rights = (perm & static_cast<int>(is_white ? CastlePerm::kWhiteQueen : CastlePerm::kBlackQueen)) != 0;
    bool king_rights = (perm & static_cast<int>(is_white ? CastlePerm::kWhiteKing : CastlePerm::kBlackKing)) != 0;

    if (queen_rights && (between::GetRayBetweenSquares(from_sqr, queen_side) & board.GetColorBB(Color::kAll)) == 0)
    {
        move_list.addMove(Move(from_sqr, queen_to, MoveType::kCastle));
    }

    if (king_rights && (between::GetRayBetweenSquares(from_sqr, king_side) & board.GetColorBB(Color::kAll)) == 0)
    {
        move_list.addMove(Move(from_sqr, king_to, MoveType::kCastle));
    }
}

template<GenType gen_type>
void GenerateKingMoves(Board& board, MoveList& move_list, size_t checker_count)
{
    Color ally_color = board.GetCurStatus().cur_color_;
    Color enemy_color = GetEnemyColor(ally_color);
    Square from_sqr = GetLsb(board.GetColoredPieceBB(ally_color, Piece::kKing));
    Bitboard king_moves = attacks::GetKingAttack(from_sqr) & ~board.GetColorBB(ally_color);
    
    // castle and captures
    if constexpr (gen_type == GenType::kNoisy || gen_type == GenType::kAll)
    {
        Bitboard noisy_moves = king_moves & board.GetColorBB(enemy_color); //captures
        while (noisy_moves != 0)
        {
            move_list.addMove(Move(from_sqr, PopLsb(noisy_moves), MoveType::kNormal));
        }

        if (checker_count == 0)
        {
            GenerateCastling(board, move_list);
        }
    }

    if constexpr (gen_type == GenType::kQuiet || gen_type == GenType::kAll)
    {
        Bitboard quiet_moves = king_moves & ~board.GetColorBB(enemy_color); // non capture quiet move.
        while (quiet_moves != 0)
        {
            move_list.addMove(Move(from_sqr, PopLsb(quiet_moves), MoveType::kNormal));
        }
    }
}

}  // namespace pseudolegal

namespace legal
{

void CleanMoveList(Board& board, MoveList& move_list);

template<GenType gen_type>
void GenerateMoves(Board& board, MoveList& move_list)
{
    Color ally_color = board.GetCurStatus().cur_color_;
    Bitboard slider_checkers_board = board.GetCurStatus().checking_squares_.at(static_cast<size_t>(ally_color));
    Bitboard leaper_checkers_board = board.GetCurStatus().leaper_checking_squares_.at(static_cast<size_t>(ally_color));
    auto slider_cherkers = BitCount(slider_checkers_board);
    auto leaper_cherkers = BitCount(leaper_checkers_board);
    Bitboard target{~0ULL};

    pseudolegal::GenerateKingMoves<gen_type>(board, move_list, slider_cherkers + leaper_cherkers);

    if (slider_cherkers + leaper_cherkers > 1) // If double check then we only generate king moves so no need to go furthur.
    { 
        CleanMoveList(board, move_list); 
        return; 
    } 


    if (slider_cherkers + leaper_cherkers == 1) // If checker we need to adjust target to be between the checker and king including checker
    {
        if (slider_cherkers != 0)
        {
            // Ray between the checker and king 
            target = between::GetRayBetweenSquares(GetLsb(board.GetColoredPieceBB(ally_color, Piece::kKing)), GetLsb(slider_checkers_board)) | slider_checkers_board; // add the checker itself as well;
        }
        else
        {   // for leapers only need to add the piece itself
            target = leaper_checkers_board;
        }
    }

    pseudolegal::GenerateMajorPieceMoves<gen_type>(board, move_list, target);

    if (ally_color == Color::kWhite)
    {
        pseudolegal::GeneratePawnMoves<gen_type, Color::kWhite>(board, move_list, target);
    }
    else
    {
        pseudolegal::GeneratePawnMoves<gen_type, Color::kBlack>(board, move_list, target);
    }

    CleanMoveList(board, move_list);
}

}  // namespace legal

} //namespace generation

