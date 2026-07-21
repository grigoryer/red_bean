#include "eval.hpp"
#include "global.hpp"

Score Evaluation::EvaluatePos(Board& board)
{
    Score score = 0;
    Color ally_color = board.GetCurStatus().cur_color_;
    Color enemy_color = GetEnemyColor(ally_color);

    score += (10 * static_cast<Score>(BitCount(board.GetColoredPieceBB(ally_color, Piece::kPawn)) - BitCount(board.GetColoredPieceBB(enemy_color, Piece::kPawn))));

    score += (30 * static_cast<Score>(BitCount(board.GetColoredPieceBB(ally_color, Piece::kKnight)) - BitCount(board.GetColoredPieceBB(enemy_color, Piece::kKnight))));

    score += (30 * static_cast<Score>(BitCount(board.GetColoredPieceBB(ally_color, Piece::kRook)) - BitCount(board.GetColoredPieceBB(enemy_color, Piece::kRook))));

    score += (50 * static_cast<Score>(BitCount(board.GetColoredPieceBB(ally_color, Piece::kBishop)) - BitCount(board.GetColoredPieceBB(enemy_color, Piece::kBishop))));

    score += (90 * static_cast<Score>(BitCount(board.GetColoredPieceBB(ally_color, Piece::kQueen)) - BitCount(board.GetColoredPieceBB(enemy_color, Piece::kQueen))));

    return score;
}