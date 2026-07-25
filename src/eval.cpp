#include "eval.hpp"
#include "attacks.hpp"
#include "eval_constants.hpp"
#include "global.hpp"
#include <cstddef>

namespace 
{
bool is_endgame = 0;


template <Piece piece>
inline Score EvaluatePiece(const Board& board, std::array<Score, kSquareCount> table)
{
    const Bitboard occ = board.GetColorBB(Color::kAll);
    Score score{};

    Bitboard white_pieces = board.GetColoredPieceBB(Color::kWhite, piece);
    while (white_pieces)
    {
        Square sqr = PopLsb(white_pieces);
        score += kPieceValues[static_cast<size_t>(piece)];
        score += table[static_cast<size_t>(sqr)];
        score += kVisionBonuses[static_cast<size_t>(piece)] * BitCount(attacks::getPieceAttack<piece>(occ, sqr, Color::kWhite));
    }

    Bitboard black_pieces = board.GetColoredPieceBB(Color::kBlack, piece);
    while (black_pieces)
    {
        Square sqr = PopLsb(black_pieces);
        score -= kPieceValues[static_cast<size_t>(piece)];
        score -= table[static_cast<size_t>(sqr) ^ 56];
        score -= kVisionBonuses[static_cast<size_t>(piece)] * BitCount(attacks::getPieceAttack<piece>(occ, sqr, Color::kBlack));
    }

    return score;
}

inline Score EvaluatePawns(const Board& board, std::array<Score, kSquareCount> pos_table)
{
    Score score = 0;
    const Bitboard white_pawns = board.GetColoredPieceBB(Color::kWhite, Piece::kPawn);
    Bitboard modified_white = white_pawns;

    const Bitboard black_pawns = board.GetColoredPieceBB(Color::kBlack, Piece::kPawn);
    Bitboard modified_black = black_pawns;

    // Need to address pawn structure, aka next to each other. Past pawns aka pawns with no one in front or next to. And king safety.
    while (modified_white)
    {
        Square sqr = PopLsb(modified_white);
        Column cur_col = GetSquareCol(sqr);

        score += kPieceValues[static_cast<size_t>(Piece::kPawn)]; // piece value 

        score += pos_table[static_cast<size_t>(sqr)]; // position of pawn

        // Get the nearby columns for the current pawn
        score += kPawnNeighborBonus * BitCount(kNearbyColumns[static_cast<size_t>(cur_col)] & white_pawns);
        
        // Add score for past pawns.
        score += (BitCount(kNearbyColumns[static_cast<size_t>(cur_col)] & ToBitboard(cur_col) & black_pawns) == 0 ? kPassedPawnBonus : 0);
    }

    while (modified_black)
    {
        Square sqr = PopLsb(modified_black);
        Column cur_col = GetSquareCol(sqr);

        score -= kPieceValues[static_cast<size_t>(Piece::kPawn)]; //piece val

        score -= pos_table[static_cast<size_t>(sqr) ^ 56]; // position of pawn

        // Get the nearby columns for the current pawn
        score -= kPawnNeighborBonus * BitCount(kNearbyColumns[static_cast<size_t>(cur_col)] & black_pawns);

        // Add score for past pawns.
        score -= (BitCount(kNearbyColumns[static_cast<size_t>(cur_col)] & ToBitboard(cur_col) & white_pawns) == 0 ? kPassedPawnBonus : 0);
    }

    return score;
}

inline Score EvaluateKings(const Board& board, std::array<Score, kSquareCount> pos_table, bool is_endgame)
{
    Score score = 0;
    const Bitboard white_pawns = board.GetColoredPieceBB(Color::kWhite, Piece::kPawn);
    const Bitboard black_pawns = board.GetColoredPieceBB(Color::kBlack, Piece::kPawn);

    Square white_ksqr = GetLsb(board.GetColoredPieceBB(Color::kWhite, Piece::kKing));
    score += pos_table[static_cast<size_t>(white_ksqr)];

    if (!is_endgame)
    {
        Column cur_col = GetSquareCol(white_ksqr);
        Row infront_row = static_cast<Row>(static_cast<int>(GetSquareRow(white_ksqr)) + 1);

        Bitboard pawn_wall = (((kNearbyColumns[static_cast<size_t>(cur_col)] | ToBitboard(cur_col))) & ToBitboard(infront_row)) & white_pawns;

        if (BitCount(pawn_wall) == 3) { score += kPawnShieldBonus; };
    }


    Square black_ksqr = GetLsb(board.GetColoredPieceBB(Color::kBlack, Piece::kKing));
    score -= pos_table[static_cast<size_t>(black_ksqr)];

    if (!is_endgame)
    {
        Column cur_col = GetSquareCol(black_ksqr);
        Row infront_row = static_cast<Row>(static_cast<int>(GetSquareRow(black_ksqr)) - 1);

        Bitboard pawn_wall = (((kNearbyColumns[static_cast<size_t>(cur_col)] | ToBitboard(cur_col))) & ToBitboard(infront_row)) & black_pawns;

        if (BitCount(pawn_wall) == 3) { score -= kPawnShieldBonus; };
    }

    return score;
}


}  // namespace


namespace evaluation
{

Score Evaluate(const Board& board)
{
    Score score = 0;

    bool is_endgame = BitCount(board.GetColorBB(Color::kAll)) <= 16;
    
    score += EvaluatePiece<Piece::kQueen>(board, kQueenPosEval);
    score += EvaluatePiece<Piece::kRook>(board, kRookPosEval);
    score += EvaluatePiece<Piece::kBishop>(board, kBishopPosEval);
    score += EvaluatePiece<Piece::kKnight>(board, kKnightPosEval);
    score += EvaluatePawns(board, is_endgame ? kPawnLatePosEval : kPawnPosEval);
    score += EvaluateKings(board, is_endgame ? kKingLatePosEval : kKingPosEval, is_endgame);
    
    return score;
}

}  // namespace evaluation