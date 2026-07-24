#include "eval.hpp"
#include "global.hpp"
#include <cstddef>

template <Piece piece>
inline Score EvaluatePosition(const Board& board)
{
    bool is_endgame = BitCount(board.GetColorBB(Color::kAll)) <= 14;

    const auto& table = [&is_endgame](const Board& b) -> const std::array<Score, kSquareCount>&
    {
        if constexpr (piece == Piece::kQueen)  { return kQueenPosEval; }
        else if constexpr (piece == Piece::kRook)   { return kRookPosEval; }
        else if constexpr (piece == Piece::kBishop) { return kBishopPosEval; }
        else if constexpr (piece == Piece::kKnight) { return kKnightPosEval; }
        else if constexpr (piece == Piece::kPawn)
        {
            return is_endgame ? kPawnLatePosEval : kPawnPosEval;
        }
        else
        {
            return is_endgame ? kKingLatePosEval : kKingPosEval;
        }
    }(board);

    Score pos_score{};

    Bitboard white_pieces = board.GetColoredPieceBB(Color::kWhite, piece);
    while (white_pieces)
    {
        pos_score += table[static_cast<size_t>(PopLsb(white_pieces))];
    }

    Bitboard black_pieces = board.GetColoredPieceBB(Color::kBlack, piece);
    while (black_pieces)
    {
        pos_score -= table[static_cast<size_t>(PopLsb(black_pieces)) ^ 56];
    }

    return pos_score;
}

inline Score EvaluatePieces(Board& board)
{
    Score score = 0;

    score += (100 * static_cast<Score>(BitCount(board.GetColoredPieceBB(Color::kWhite, Piece::kPawn)) - BitCount(board.GetColoredPieceBB(Color::kBlack, Piece::kPawn))));

    score += (300 * static_cast<Score>(BitCount(board.GetColoredPieceBB(Color::kWhite, Piece::kKnight)) - BitCount(board.GetColoredPieceBB(Color::kBlack, Piece::kKnight))));
    
    score += (300 * static_cast<Score>(BitCount(board.GetColoredPieceBB(Color::kWhite, Piece::kBishop)) - BitCount(board.GetColoredPieceBB(Color::kBlack, Piece::kBishop))));
    
    score += (500 * static_cast<Score>(BitCount(board.GetColoredPieceBB(Color::kWhite, Piece::kRook)) - BitCount(board.GetColoredPieceBB(Color::kBlack, Piece::kRook))));

    score += (900 * static_cast<Score>(BitCount(board.GetColoredPieceBB(Color::kWhite, Piece::kQueen)) - BitCount(board.GetColoredPieceBB(Color::kBlack, Piece::kQueen))));

    return score;
}


namespace evaluation
{

Score Evaluate(Board& board)
{
    Score score = EvaluatePieces(board);

    score += EvaluatePosition<Piece::kQueen>(board);
    score += EvaluatePosition<Piece::kRook>(board);
    score += EvaluatePosition<Piece::kBishop>(board);
    score += EvaluatePosition<Piece::kKnight>(board);
    score += EvaluatePosition<Piece::kPawn>(board);
    score += EvaluatePosition<Piece::kKing>(board);
    
    return score;
}

}  // namespace evaluation