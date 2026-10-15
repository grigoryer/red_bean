#include "eval.hpp"
#include <algorithm>
#include "attacks.hpp"
#include "eval_constants.hpp"
#include "global.hpp"
#include "nnue.hpp"
#include <cstddef>

namespace
{

struct EvalPair
{
    int mg{};
    int eg{};
};


template <Color color, Piece piece>
inline void EvaluateMaterialPiece(const Board& board, EvalPair& acc, int& non_pawn_material, Bitboard occ, Bitboard pawn_attacks, Bitboard enemy_king_zone, int& king_attackers, Score& value_of_attacks)
{
    constexpr int kSign = (color == Color::kBlack) ? -1 : 1;
    
    const Bitboard mobility_area = ~(board.GetColorBB(color) | pawn_attacks);

    Bitboard piece_bb = board.GetColoredPieceBB(color, piece);
    const int piece_count = static_cast<int>(BitCount(piece_bb));

    // Phase-independent terms same contribution to both mg and eg.
    int flat = kPieceValues[static_cast<size_t>(piece)] * piece_count;

    int king_safety_score = 0;

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

    non_pawn_material += kPieceValues[static_cast<size_t>(piece)] * piece_count;

    acc.mg += kSign * flat;
    acc.eg += kSign * flat;

    int local_king_attacker_count{};

    while (piece_bb)
    {
        Square sqr = PopLsb(piece_bb);
        Square pst_sqr = sqr;

        if constexpr (color == Color::kBlack) { pst_sqr = static_cast<Square>(static_cast<int>(sqr) ^ 56); }

        // attacking king zone.
        Bitboard piece_attack = attacks::GetPieceAttack<piece>(occ, sqr, color);

        int king_attacked_squares_count = BitCount(piece_attack & enemy_king_zone);
        if (king_attacked_squares_count != 0)
        {
            local_king_attacker_count++;
            value_of_attacks += kAttackingKingValues[static_cast<size_t>(piece)] * king_attacked_squares_count;            
        }

        int vision_count = BitCount(piece_attack & mobility_area);
        acc.mg += kSign * kMG_VisionBonuses[static_cast<size_t>(piece)] * vision_count;
        acc.eg += kSign * kEG_VisionBonuses[static_cast<size_t>(piece)] * vision_count;

        acc.mg += kSign * GetPSTScore<piece, Phase::kMg>(pst_sqr);
        acc.eg += kSign * GetPSTScore<piece, Phase::kEg>(pst_sqr);
    }

    king_attackers += local_king_attacker_count;
}


template<Color color>
inline Bitboard KingZoneAttacking(Square ksqr)
{
    Bitboard zone = 0ULL;
    zone = attacks::GetKingAttack(ksqr);
    
    if constexpr (color == Color::kWhite)
    {
        zone |= (zone << 8);
    }
    else if constexpr (color == Color::kBlack)
    {
        zone |= (zone >> 8);
    }

    return zone;
}

template<Color color>
inline void KingSafety(Board& board, Bitboard ally_pawns, EvalPair& acc, Square ksqr, Bitboard occ)
{
    constexpr int kSign = (color == Color::kBlack) ? -1 : 1;
    
    // King position
    Square pst_sqr = ksqr;
    if constexpr (color == Color::kBlack) { pst_sqr = static_cast<Square>(static_cast<int>(ksqr) ^ 56); }
    
    // How visible a king is, negative score so that we promote it to hide
    acc.mg += kSign * BitCount(attacks::GetQueenAttack(occ, ksqr) & ~board.GetColorBB(color))
        * kMG_VisionBonuses[static_cast<size_t>(Piece::kKing)];

    //PST
    acc.mg += kSign * GetPSTScore<Piece::kKing, Phase::kMg>(pst_sqr);
    acc.eg += kSign * GetPSTScore<Piece::kKing, Phase::kEg>(pst_sqr);

    // Pawn shield
    const int row = static_cast<int>(GetSquareRow(ksqr));
    int shield_count = 0;
    auto col_index = static_cast<size_t>(GetSquareCol(ksqr));
    Bitboard cols_infront = kBandColumns[col_index];

    // check the rows in front of king
    for (int i{1}; i < kPawnShieldRowsInfront; ++i)
    {
        const int shield_idx = row + (i * kSign);
        // Safety check for king. If row will exceed the bounds of board then break.
        if (shield_idx < 0 || shield_idx > 7) { break; }

        const Row row_infront = static_cast<Row>(shield_idx);

        // if king is too far up dont apply.
        if constexpr (color == Color::kBlack) {  if ( row_infront < Row::_5) { break; } } 
        if constexpr (color == Color::kWhite) {  if ( row_infront > Row::_4) { break; } }

        Bitboard row_infront_bb = cols_infront & ToBitboard(row_infront);

        // Pawn Shield compute the 3 squares in front on corresponding row and count amount of square blockers.
        int sheild_level_count = BitCount(ally_pawns & row_infront_bb);
        shield_count += sheild_level_count;
        acc.mg += kSign * (kPawnShieldBonus[i] * sheild_level_count);
    }
    if (shield_count == 0) { acc.mg += kSign * kPawnShieldPenality; }


}

template<Color color>
inline void PawnStructure(Bitboard ally_pawns, EvalPair& acc, Bitboard enemy_pawns, Bitboard enemy_pawn_attacks)
{
    constexpr int kSign = (color == Color::kBlack) ? -1 : 1;
    constexpr int kRowStep = (color == Color::kBlack) ? -1 : 1;

    const int pawn_material = kPieceValues[static_cast<size_t>(Piece::kPawn)]
                            * static_cast<int>(BitCount(ally_pawns));

    acc.mg += kSign * pawn_material;
    acc.eg += kSign * pawn_material;

    // No pawn penality
    if (ally_pawns == 0) 
    { 
        acc.mg += kSign * kNoPawnsPenalty; 
        acc.eg += kSign * kNoPawnsPenalty; 
    }

    Bitboard ally_pawns_copy = ally_pawns;
    EvalPair flat_score{};

    while (ally_pawns_copy)
    {
        Square sqr = PopLsb(ally_pawns_copy);
        Column col = GetSquareCol(sqr);
        Row row = GetSquareRow(sqr);
        size_t row_idx = static_cast<size_t>(row);
        size_t col_idx = static_cast<size_t>(col);
        size_t bonus_row = (color == Color::kBlack) ? (7 - row_idx) : row_idx;

        // PST FOR PAWN 
        Square pst_sqr = sqr;
        if constexpr (color == Color::kBlack) { pst_sqr = static_cast<Square>(static_cast<int>(sqr) ^ 56); }

        flat_score.mg += GetPSTScore<Piece::kPawn, Phase::kMg>(pst_sqr);
        flat_score.eg += GetPSTScore<Piece::kPawn, Phase::kEg>(pst_sqr);

        Bitboard rows_ahead = kRowsInfront[static_cast<size_t>(color)][row_idx];
        Bitboard behind_or_level_mask = ~rows_ahead;
        Bitboard same_file_ahead = enemy_pawns & kColumnsBitboards[col_idx] & rows_ahead;
        Bitboard adjacent_ahead = enemy_pawns & kNearbyColumns[col_idx] & rows_ahead;
        Bitboard ally_adjacent_files = ally_pawns & kNearbyColumns[col_idx];
        Bitboard ally_same_file = ally_pawns & kColumnsBitboards[col_idx];

        // add bonus for passed pawn, will give same bonus.
        // Passed pawn = no enemy pawns in cur or side rows and that are infront.
        bool passed_pawn = (same_file_ahead == 0) && (adjacent_ahead == 0);

        if (passed_pawn) 
        {   // Passed Pawn
            flat_score.mg += kPassedPawnBonus[static_cast<size_t>(Phase::kMg)][bonus_row];
            flat_score.eg += kPassedPawnBonus[static_cast<size_t>(Phase::kEg)][bonus_row];
        }
        else 
        {   // Backward Pawn Penality
            int stop_row_idx = static_cast<int>(row_idx) + kRowStep;
            if (stop_row_idx >= 0 && stop_row_idx <= 7)
            {
                Square stop_sqr = static_cast<Square>((stop_row_idx * 8) + static_cast<int>(col_idx));
                if ((enemy_pawn_attacks & ToBitboard(stop_sqr)) != 0)
                {
                    if ((ally_adjacent_files != 0) && (ally_adjacent_files & behind_or_level_mask) == 0)
                    {
                        flat_score.mg += kBackwardPawnPenalty[static_cast<size_t>(Phase::kMg)];
                        flat_score.eg += kBackwardPawnPenalty[static_cast<size_t>(Phase::kEg)];
                    }
                }
            }
        }
        // Candidate Passed pawn
        if (!passed_pawn && same_file_ahead == 0)
        {
            size_t sentries = BitCount(adjacent_ahead);
            size_t helpers = BitCount(ally_adjacent_files & behind_or_level_mask);

            if (helpers >= sentries)
            {
                flat_score.mg += kCandidatePassedPawnBonus[static_cast<size_t>(Phase::kMg)][bonus_row];
                flat_score.eg += kCandidatePassedPawnBonus[static_cast<size_t>(Phase::kEg)][bonus_row];
            }
        }

        // Nearby pawn structure bonuses and penalties

        // Isolated Pawn Penality
        if (ally_adjacent_files == 0)
        {
            flat_score.mg += kIsolatedPawnPenalty[static_cast<size_t>(Phase::kMg)];
            flat_score.eg += kIsolatedPawnPenalty[static_cast<size_t>(Phase::kEg)];
        }

        // Doubled pawn penality
        if ((ally_same_file & rows_ahead) != 0)
        {
            flat_score.mg += kDoubledPawnPenalty[static_cast<size_t>(Phase::kMg)];
            flat_score.eg += kDoubledPawnPenalty[static_cast<size_t>(Phase::kEg)];
        }

        size_t chain_count = BitCount(ally_pawns & attacks::GetPawnAttack(color, sqr));
        flat_score.mg += static_cast<int>(chain_count) * kChainPawnBonus[static_cast<size_t>(Phase::kMg)][bonus_row];
        flat_score.eg += static_cast<int>(chain_count) * kChainPawnBonus[static_cast<size_t>(Phase::kEg)][bonus_row];

        size_t phalanx_count = BitCount(ally_adjacent_files & ToBitboard(row));
        flat_score.mg += static_cast<int>(phalanx_count) * kPhalanxPawnBonus[static_cast<size_t>(Phase::kMg)][bonus_row];
        flat_score.eg += static_cast<int>(phalanx_count) * kPhalanxPawnBonus[static_cast<size_t>(Phase::kEg)][bonus_row];
    }

    acc.mg += flat_score.mg * kSign;
    acc.eg += flat_score.eg * kSign;
}

}  // namespace


Score HandCraftedEvaluation(Board& board)
{
    EvalPair acc{};
    int white_material{};
    int black_material{};

    Bitboard occ = board.GetColorBB(Color::kAll);
    Bitboard white_pawns = board.GetColoredPieceBB(Color::kWhite, Piece::kPawn);
    Bitboard black_pawns = board.GetColoredPieceBB(Color::kBlack, Piece::kPawn);

    Bitboard white_pawn_attacks = attacks::GetPawnAttacks<Color::kWhite>(board);
    Bitboard black_pawn_attacks = attacks::GetPawnAttacks<Color::kBlack>(board);

    Score white_value_of_attacks{}; // unsafe score
    Bitboard white_safety_zone = KingZoneAttacking<Color::kWhite>(GetLsb(board.GetColoredPieceBB(Color::kWhite, Piece::kKing)));
    int white_king_attackers_count{};

    Score black_value_of_attacks{}; // unsafe score
    Bitboard black_safety_zone = KingZoneAttacking<Color::kBlack>(GetLsb(board.GetColoredPieceBB(Color::kBlack, Piece::kKing)));
    int black_king_attackers_count{};

    EvaluateMaterialPiece<Color::kWhite, Piece::kQueen>(board, acc, white_material, occ, black_pawn_attacks, 
                            black_safety_zone, black_king_attackers_count, black_value_of_attacks);

    EvaluateMaterialPiece<Color::kWhite, Piece::kRook>(board, acc, white_material, occ, black_pawn_attacks, 
                            black_safety_zone, black_king_attackers_count, black_value_of_attacks);

    EvaluateMaterialPiece<Color::kWhite, Piece::kBishop>(board, acc, white_material, occ, black_pawn_attacks,  
                            black_safety_zone, black_king_attackers_count, black_value_of_attacks);

    EvaluateMaterialPiece<Color::kWhite, Piece::kKnight>(board, acc, white_material, occ, black_pawn_attacks, 
                            black_safety_zone, black_king_attackers_count, black_value_of_attacks);
          
    // BLACK SIDE

    EvaluateMaterialPiece<Color::kBlack, Piece::kQueen>(board, acc, black_material, occ, white_pawn_attacks, 
                            white_safety_zone, white_king_attackers_count, white_value_of_attacks);

    EvaluateMaterialPiece<Color::kBlack, Piece::kRook>(board, acc, black_material, occ, white_pawn_attacks, 
                            white_safety_zone, white_king_attackers_count, white_value_of_attacks);

    EvaluateMaterialPiece<Color::kBlack, Piece::kBishop>(board, acc, black_material, occ, white_pawn_attacks, 
                            white_safety_zone, white_king_attackers_count, white_value_of_attacks);

    EvaluateMaterialPiece<Color::kBlack, Piece::kKnight>(board, acc, black_material, occ, white_pawn_attacks, 
                            white_safety_zone, white_king_attackers_count, white_value_of_attacks);


    acc.mg += black_value_of_attacks * kAttackCountWeight[std::min(7, black_king_attackers_count)] / 100;
    acc.mg -= white_value_of_attacks * kAttackCountWeight[std::min(7, white_king_attackers_count)] / 100;
    
    PawnStructure<Color::kWhite>(white_pawns, acc, black_pawns, black_pawn_attacks);
    PawnStructure<Color::kBlack>(black_pawns, acc, white_pawns, white_pawn_attacks);

    KingSafety<Color::kWhite>(board, white_pawns, acc, (GetLsb(board.GetColoredPieceBB(Color::kWhite, Piece::kKing))), occ);
    KingSafety<Color::kBlack>(board, black_pawns, acc, (GetLsb(board.GetColoredPieceBB(Color::kBlack, Piece::kKing))), occ);

    const int phase = std::clamp<int8_t>(board.GetCurStatus().phase_num, static_cast<int8_t>(0), kMaxPhase);
    int score = ((acc.mg * phase) + (acc.eg * (kMaxPhase - phase))) / kMaxPhase;

    // Trade-down: the side ahead in material gains more from each piece leaving the board.
    const int advantage = white_material - black_material;
    if (advantage != 0)
    {
        const int remaining = white_material + black_material;
        const int traded = std::clamp(kStartNonPawnMaterial - remaining, 0, kStartNonPawnMaterial);
        score += (advantage * kTradeDownBonus * traded) / (kStartNonPawnMaterial * 100);
    }

    score += board.GetCurStatus().cur_color_ == Color::kBlack ? -kTempoBonus : kTempoBonus; // tempo
    return static_cast<Score>(score);
}


namespace evaluation
{
    Score Evaluate(Board& board, Color color)
    {
        if (color == Color::kWhite)
        {
            return Nnue::Forward(board.acc.white_accumulator_);
        }

        return Nnue::Forward(board.acc.black_accumulator_);
    }
}  // namespace evaluation