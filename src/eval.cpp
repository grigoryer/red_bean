#include "eval.hpp"
#include <algorithm>
#include "attacks.hpp"
#include "eval_constants.hpp"
#include "global.hpp"
#include <cstddef>

namespace 
{

struct EvalPair
{
    int mg{};
    int eg{};
};

template <Color color, Piece piece>
inline void EvaluateMaterialPiece(const Board& board, EvalPair& acc, int& non_pawn_material, Bitboard occ, Bitboard pawn_attacks)
{
    constexpr int kSign = (color == Color::kBlack) ? -1 : 1;
    
    const Bitboard mobility_area = ~(board.GetColorBB(color) | pawn_attacks);

    Bitboard piece_bb = board.GetColoredPieceBB(color, piece);
    const int piece_count = static_cast<int>(BitCount(piece_bb));

    // Phase-independent terms same contribution to both mg and eg.
    int flat = kPieceValues[static_cast<size_t>(piece)] * piece_count;

    if constexpr (piece == Piece::kBishop)
    {
        if (piece_count > 1) { flat += kBishopPairBonus; }
    }
    else if constexpr (piece == Piece::kKnight)
    {
        if (piece_count > 1) { flat += kKnightPairPenalty; }
    }
    else if constexpr (piece == Piece::kRook)
    {
        if (piece_count > 1) { flat += kRookPairPenalty; }
    }
    else if constexpr (piece == Piece::kPawn)
    {
        if (piece_count == 0) { flat += kNoPawnsPenalty; }
    }

    if constexpr (piece != Piece::kPawn)
    {
        non_pawn_material += kPieceValues[static_cast<size_t>(piece)] * piece_count;
    }

    acc.mg += kSign * flat;
    acc.eg += kSign * flat;

    while (piece_bb)
    {
        Square sqr = PopLsb(piece_bb);
        Square pst_sqr = sqr;

        if constexpr (color == Color::kBlack) { pst_sqr = static_cast<Square>(static_cast<int>(sqr) ^ 56); }

        if constexpr (piece != Piece::kPawn)
        {
            int vision_count = BitCount(attacks::getPieceAttack<piece>(occ, sqr, color) & mobility_area);
    
            acc.mg += kSign * kMG_VisionBonuses[static_cast<size_t>(piece)] * vision_count;
            acc.eg += kSign * kEG_VisionBonuses[static_cast<size_t>(piece)] * vision_count;
        }

        acc.mg += kSign * GetPSTScore<piece, Phase::kMg>(pst_sqr);
        acc.eg += kSign * GetPSTScore<piece, Phase::kEg>(pst_sqr);
    }
}


template<Color color>
inline void KingSafety(Board& board, Bitboard ally_pawns, EvalPair& acc, Square ksqr, Bitboard occ)
{
    constexpr int kSign = (color == Color::kBlack) ? -1 : 1;
    
    // King position
    Square pst_sqr = ksqr;
    if constexpr (color == Color::kBlack) { pst_sqr = static_cast<Square>(static_cast<int>(ksqr) ^ 56); }
    
    // How visible a king is.
    acc.mg += kSign * BitCount(attacks::GetQueenAttack(occ, ksqr) & ~board.GetColorBB(color))
        * kMG_VisionBonuses[static_cast<size_t>(Piece::kKing)];

    //PST
    acc.mg += kSign * GetPSTScore<Piece::kKing, Phase::kMg>(pst_sqr);
    acc.eg += kSign * GetPSTScore<Piece::kKing, Phase::kEg>(pst_sqr);

    // Pawn shield
    constexpr int kDir = (color == Color::kBlack) ? -1 : 1;
    const int row = static_cast<int>(GetSquareRow(ksqr)) + kDir;
    
    if (row < static_cast<int>(Row::_2) || row > static_cast<int>(Row::_7))
    {
        acc.mg += kSign * kPawnShieldPenality;
        return;
    }

    const Row row_infront = static_cast<Row>(row);

    Column col = std::clamp(GetSquareCol(ksqr), Column::B, Column::G);
    ally_pawns &= ((kNearbyColumns[static_cast<size_t>(col)] | ToBitboard(col)) & ToBitboard(row_infront));

    if (BitCount(ally_pawns) >= 3) { acc.mg += kSign * kPawnShieldBonus; }
    else { acc.mg += kSign * kPawnShieldPenality; }
}

inline Score EvaluateMaterial(Board& board)
{
    EvalPair acc{};
    int white_material{};
    int black_material{};

    Bitboard occ = board.GetColorBB(Color::kAll);
    Bitboard white_pawns = board.GetColoredPieceBB(Color::kWhite, Piece::kPawn);
    Bitboard black_pawns = board.GetColoredPieceBB(Color::kBlack, Piece::kPawn);

    Bitboard white_pawn_attacks = attacks::GetPawnAttacks<Color::kWhite>(board);
    Bitboard black_pawn_attacks = attacks::GetPawnAttacks<Color::kBlack>(board);

    EvaluateMaterialPiece<Color::kWhite, Piece::kQueen>(board, acc, white_material, occ, black_pawn_attacks);
    EvaluateMaterialPiece<Color::kWhite, Piece::kRook>(board, acc, white_material, occ, black_pawn_attacks);
    EvaluateMaterialPiece<Color::kWhite, Piece::kBishop>(board, acc, white_material, occ, black_pawn_attacks);
    EvaluateMaterialPiece<Color::kWhite, Piece::kKnight>(board, acc, white_material, occ, black_pawn_attacks);
    EvaluateMaterialPiece<Color::kWhite, Piece::kPawn>(board, acc, white_material, occ, black_pawn_attacks);

    EvaluateMaterialPiece<Color::kBlack, Piece::kQueen>(board, acc, black_material, occ, white_pawn_attacks);
    EvaluateMaterialPiece<Color::kBlack, Piece::kRook>(board, acc, black_material, occ, white_pawn_attacks);
    EvaluateMaterialPiece<Color::kBlack, Piece::kBishop>(board, acc, black_material, occ, white_pawn_attacks);
    EvaluateMaterialPiece<Color::kBlack, Piece::kKnight>(board, acc, black_material, occ, white_pawn_attacks);
    EvaluateMaterialPiece<Color::kBlack, Piece::kPawn>(board, acc, black_material, occ, white_pawn_attacks);

    KingSafety<Color::kWhite>(board, white_pawns, acc, (GetLsb(board.GetColoredPieceBB(Color::kWhite, Piece::kKing))), occ);
    KingSafety<Color::kBlack>(board, black_pawns, acc, (GetLsb(board.GetColoredPieceBB(Color::kBlack, Piece::kKing))), occ);

    const int phase = std::clamp(board.GetCurStatus().phase_num, 0, kMaxPhase);
    int score = ((acc.mg * phase) + (acc.eg * (kMaxPhase - phase))) / kMaxPhase;

    // Trade-down: the side ahead in material gains more from each piece leaving the board.


    const int advantage = white_material - black_material;
    if (advantage != 0)
    {
        const int remaining = white_material + black_material;
        const int traded = std::clamp(kStartNonPawnMaterial - remaining, 0, kStartNonPawnMaterial);
        score += (advantage * kTradeDownBonus * traded) / (kStartNonPawnMaterial * 100);
    }

    return static_cast<Score>(score);
}

}  // namespace


namespace evaluation
{

Score Evaluate(Board& board)
{
    Score score = 0;
    score += EvaluateMaterial(board);
        
    
    return score;
}

}  // namespace evaluation