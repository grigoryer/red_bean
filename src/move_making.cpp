#include "checking.hpp"
#include "move.hpp"
#include "global.hpp"
#include "board.hpp"
#include "zobrist.hpp"
#include <cstdlib>
#include <cstddef>



namespace moving
{

namespace 
{

constexpr std::array<uint8_t, kSquareCount> InitializeCastleMaskArray()
{
    std::array<uint8_t, kSquareCount> mask{};
    constexpr uint8_t kAllRights = static_cast<uint8_t>(CastlePerm::kWhiteBoth)
                                    | static_cast<uint8_t>(CastlePerm::kBlackBoth);
    for (auto& m : mask)
    {
        m = kAllRights;
    }
    // Touching a corner kills that one right; touching a king square kills both for that color.
    mask.at(ToIndex(Square::a1)) &= ~static_cast<uint8_t>(CastlePerm::kWhiteQueen);
    mask.at(ToIndex(Square::h1)) &= ~static_cast<uint8_t>(CastlePerm::kWhiteKing);
    mask.at(ToIndex(Square::e1)) &= ~static_cast<uint8_t>(CastlePerm::kWhiteBoth);
    mask.at(ToIndex(Square::a8)) &= ~static_cast<uint8_t>(CastlePerm::kBlackQueen);
    mask.at(ToIndex(Square::h8)) &= ~static_cast<uint8_t>(CastlePerm::kBlackKing);
    mask.at(ToIndex(Square::e8)) &= ~static_cast<uint8_t>(CastlePerm::kBlackBoth);
    return mask;
}

inline constexpr std::array<uint8_t, kSquareCount> kCastleMaskArray = InitializeCastleMaskArray();

inline void RemovePieceNoHash(Board &board, Piece piece, Square sqr, Color color)
{
    board.RemovePieceBB(piece, sqr);
    board.RemoveColorBB(color, sqr);
    board.RemoveColorBB(Color::kAll, sqr);
    board.SetPieceOnSquare(Piece::kNone, sqr);
}
inline void RemovePiece(Board &board, Piece piece, Square sqr, Color color, Hash& hash)
{
    RemovePieceNoHash(board, piece, sqr, color);
    hash ^= Zobrist::ZobPiece(piece, sqr, color);
}

inline void SetPieceNoHash(Board &board, Piece piece, Square sqr, Color color)
{
    board.SetPieceBB(piece, sqr);
    board.SetColorBB(color, sqr);
    board.SetColorBB(Color::kAll, sqr);
    board.SetPieceOnSquare(piece, sqr);
}

inline void SetPiece(Board &board, Piece piece, Square sqr, Color color, Hash& hash)
{
    SetPieceNoHash(board, piece, sqr, color);
    hash ^= Zobrist::ZobPiece(piece, sqr, color);
}

inline void MovePieceNoHash(Board &board, Piece piece, Square from_sqr, Color color, Square to_sqr)
{
    RemovePieceNoHash(board, piece, from_sqr, color);
    SetPieceNoHash(board, piece, to_sqr, color);
}

inline void MovePiece(Board &board, Piece piece, Square from_sqr, Color color, Square to_sqr, Hash& hash)
{
    RemovePiece(board, piece, from_sqr, color, hash);
    SetPiece(board, piece, to_sqr, color, hash);
}

inline void ChangeColor(Status& status)
{
    status.hash_ ^= Zobrist::ZobColor(status.cur_color_);
    status.cur_color_ = static_cast<Color>(static_cast<int>(status.cur_color_) ^ 1);
    status.hash_ ^= Zobrist::ZobColor(status.cur_color_);

    if (status.cur_color_ == Color::kWhite) { status.full_move_++; }
}

inline void SetEpsquare(Status& status, Square sqr)
{
    status.hash_ ^= Zobrist::ZobEnpassant(status.enpassant_);
    status.enpassant_ = sqr;
    status.hash_ ^= Zobrist::ZobEnpassant(status.enpassant_);
}


inline void SetCastlPerm(Status& status, CastlePerm perm)
{
    status.hash_ ^= Zobrist::ZobCastling(status.castling_);
    status.castling_ = perm;
    status.hash_ ^= Zobrist::ZobCastling(status.castling_);
}

} // namespace



// Needs to 
void MakeMove(Board& board, Move move)
{   
    auto& status = board.CreateNewStatus();
    Square from_sqr{move.GetFrom()};
    Square to_sqr{move.GetTo()};

    Piece piece{board.GetPieceOnSquare(from_sqr)};
    status.captured_piece_ = {board.GetPieceOnSquare(to_sqr)};

    Color ally_color{status.cur_color_};
    Color enemy_color{GetEnemyColor(ally_color)};
    Hash& hash = status.hash_;

    bool reset_halfclock{false};
    status.half_move_++;
    SetEpsquare(status, Square::kNoSquare);

    auto new_perm = static_cast<CastlePerm>(static_cast<uint8_t>(status.castling_) & kCastleMaskArray.at(ToIndex(from_sqr)) & kCastleMaskArray.at(ToIndex(to_sqr)));
    if (new_perm != status.castling_)
    {
        SetCastlPerm(status, new_perm);
    }

    if (status.captured_piece_ != Piece::kNone) // remove captured piece color.
    {
        RemovePiece(board, status.captured_piece_, to_sqr, enemy_color, hash);
        reset_halfclock = true; // reset the half move since capture.
    }

    if (move.GetType() != MoveType::kNormal) // not normal then do something extra, (castle, enpassant, promo)
    {
        // Mutually exclusive only need to go one path
        if (move.IsCastle())
        {
            bool col_a_rook = (GetSquareCol(to_sqr) == Column::C);

            Square rook_to = col_a_rook ? Square::d1 : Square::f1;
            Square rook_from = col_a_rook ? Square::a1 : Square::h1;

            rook_to = static_cast<Square>(static_cast<int>(rook_to) + (ally_color == Color::kWhite ? 0 : 56));  // 56 == a8 or last row.
            rook_from = static_cast<Square>(static_cast<int>(rook_from) + (ally_color == Color::kWhite ? 0 : 56));

            MovePiece(board, Piece::kRook, rook_from, ally_color, rook_to, hash);
        }
        else if (move.IsEnPassant())
        {
            int shift = (ally_color == Color::kWhite ? -8 : 8);
            auto captured_square{static_cast<Square>(static_cast<int>(to_sqr) + shift)};
            RemovePiece(board, Piece::kPawn, captured_square, enemy_color, hash);
        }
        else if (move.IsPromotion())
        {
            RemovePiece(board, Piece::kPawn, from_sqr, ally_color, hash);
            SetPiece(board, move.GetPromo(), to_sqr, ally_color, hash);
            ChangeColor(status);
            between::SetCheckersAndBlockers(board, status);
            status.half_move_ = 0;
            return;
        }
    }

    // need to update en passant permission
    if (piece == Piece::kPawn) 
    {
        reset_halfclock = true; // reset the half move since capture.
        if (std::abs(static_cast<int>(to_sqr) - static_cast<int>(from_sqr)) == 16)
        {
            int ep_shift = (ally_color == Color::kWhite ? -8 : 8);
            auto ep_sqr =static_cast<Square>(static_cast<int>(to_sqr) + ep_shift);
            SetEpsquare(status, ep_sqr);
        }
    }

    if (reset_halfclock) { status.half_move_ = 0; }
    MovePiece(board, piece, from_sqr, ally_color, to_sqr, hash);
    ChangeColor(status);
}



void UndoMove(Board& board, Move move)
{
    // Status on top of the stack is the one make_move created for this move.
    auto& status = board.GetCurStatus();
    Square from_sqr{move.GetFrom()};
    Square to_sqr{move.GetTo()};

    // ChangeColor already flipped, so the side that made the move is the enemy of cur_color_.
    Color ally_color{GetEnemyColor(status.cur_color_)};
    Color enemy_color{status.cur_color_};

    // Put the moved piece back on its origin square.
    if (move.IsPromotion())
    {
        RemovePieceNoHash(board, move.GetPromo(), to_sqr, ally_color);
        SetPieceNoHash(board, Piece::kPawn, from_sqr, ally_color);
    }
    else
    {
        Piece piece{board.GetPieceOnSquare(to_sqr)};
        MovePieceNoHash(board, piece, to_sqr, ally_color, from_sqr);
    }

    if (move.IsCastle())
    {
        // Same rook squares as make_move, reversed.
        bool col_a_rook = (GetSquareCol(to_sqr) == Column::C);
        Square rook_to = col_a_rook ? Square::d1 : Square::f1;
        Square rook_from = col_a_rook ? Square::a1 : Square::h1;
        rook_to = static_cast<Square>(static_cast<int>(rook_to) + (ally_color == Color::kWhite ? 0 : 56));
        rook_from = static_cast<Square>(static_cast<int>(rook_from) + (ally_color == Color::kWhite ? 0 : 56));
        MovePieceNoHash(board, Piece::kRook, rook_to, ally_color, rook_from);
    }
    else if (move.IsEnPassant())
    {
        // The victim pawn was not on to_sqr; recompute its square.
        int shift = (ally_color == Color::kWhite ? -8 : 8);
        auto captured_square{static_cast<Square>(static_cast<int>(to_sqr) + shift)};
        SetPieceNoHash(board, Piece::kPawn, captured_square, enemy_color);
    }
    else if (status.captured_piece_ != Piece::kNone)
    {
        // Covers normal captures and promotion-captures alike.
        SetPieceNoHash(board, status.captured_piece_, to_sqr, enemy_color);
    }

    // Everything else (hash, castling, ep, clocks, checkers/blockers) comes back with the pop.
    board.PopStatus();
}



}  // namespace moving