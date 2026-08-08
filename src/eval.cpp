#include "eval.hpp"
#include <algorithm>
#include "attacks.hpp"
#include "eval_constants.hpp"
#include "global.hpp"
#include <cstddef>

using namespace spsa_eval;  // swap to game_eval for the frozen (constexpr) set

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
            int vision_count = BitCount(attacks::GetPieceAttack<piece>(occ, sqr, color) & mobility_area);
    
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

inline void PawnMajority(Bitboard white_pawns, Bitboard black_pawns, EvalPair& acc)
{
    auto score_sector = [&](Bitboard sector_mask)
    {
        int white_count = static_cast<int>(BitCount(white_pawns & sector_mask));
        int black_count = static_cast<int>(BitCount(black_pawns & sector_mask));
        int margin = std::abs(white_count - black_count);
        if (margin == 0) { return; }

        margin = std::min(margin, 4); // clamp to table size
        int sign = (white_count > black_count) ? 1 : -1;

        acc.mg += sign * kPawnMajorityBonus[static_cast<size_t>(Phase::kMg)][static_cast<size_t>(margin)];
        acc.eg += sign * kPawnMajorityBonus[static_cast<size_t>(Phase::kEg)][static_cast<size_t>(margin)];
    };

    score_sector(kQueensideMask);
    score_sector(kKingsideMask);
}

template<Color color>
inline void PawnIslands(Bitboard ally_pawns, EvalPair& acc)
{
    if (!ally_pawns) { return; }

    constexpr int kSign = (color == Color::kBlack) ? -1 : 1;

    int island_count = 0;
    bool in_island = false;

    for (size_t file = 0; file < kColumnCount; ++file)
    {
        bool file_occupied = (ally_pawns & kColumnsBitboards[file]) != 0;
        if (file_occupied)
        {
            if (!in_island)
            {
                ++island_count;
                in_island = true;
            }
        }
        else
        {
            in_island = false;
        }
    }

    acc.mg += kSign * kPawnIslandsPenalty[static_cast<size_t>(Phase::kMg)][static_cast<size_t>(island_count)];
    acc.eg += kSign * kPawnIslandsPenalty[static_cast<size_t>(Phase::kEg)][static_cast<size_t>(island_count)];
}

template<Color color>
inline void PawnStructure(Bitboard ally_pawns, EvalPair& acc, Bitboard enemy_pawns, Bitboard enemy_pawn_attacks)
{
    constexpr int kSign = (color == Color::kBlack) ? -1 : 1;
    constexpr int kRowStep = (color == Color::kBlack) ? -1 : 1;

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

    PawnIslands<color>(ally_pawns, acc);
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

    PawnStructure<Color::kWhite>(white_pawns, acc, black_pawns, black_pawn_attacks);
    PawnStructure<Color::kBlack>(black_pawns, acc, white_pawns, white_pawn_attacks);

    KingSafety<Color::kWhite>(board, white_pawns, acc, (GetLsb(board.GetColoredPieceBB(Color::kWhite, Piece::kKing))), occ);
    KingSafety<Color::kBlack>(board, black_pawns, acc, (GetLsb(board.GetColoredPieceBB(Color::kBlack, Piece::kKing))), occ);

    PawnMajority(white_pawns, black_pawns, acc);

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

    //score += board.GetCurStatus().cur_color_ == Color::kBlack ? -kTempoBonus : kTempoBonus; // tempo
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