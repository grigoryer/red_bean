#include "search.hpp"
#include "attacks.hpp"
#include "checking.hpp"
#include "eval_constants.hpp"
#include "move_generation.hpp"
#include "move_making.hpp"
#include "eval.hpp"
#include "uci.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <limits>

namespace 
{
inline int PieceValue(Piece piece)
{
    return (piece == Piece::kNone) ? 0 : kPieceMoveWorth[static_cast<size_t>(piece)];
}

void ApplyGravity(MoveScore& entry, MoveScore bonus)
{
    entry += bonus - entry * std::abs(bonus) / kHistoryMax; // gravity formula
}

}  // namespace

Search::Search(std::atomic<bool>& stop_flag, std::shared_ptr<TTable> tt)
    : stop_flag_(stop_flag), ttable_(std::move(tt)) { }


void Search::UpdatePv(uint8_t ply, ExtMove move)
{
    pv_table_[ply][0] = move;

    const int child_len = (ply + 1 < kMaxPly) ? pv_length_[ply + 1] : 0;
    const int copy_len  = std::min(child_len, kMaxPly - 1 - static_cast<int>(ply));

    for (int i{}; i < copy_len; ++i)
    {
        pv_table_[ply][i + 1] = pv_table_[ply + 1][i];
    }
    pv_length_[ply] = copy_len + 1;
}

void Search::AgeHistory()
{
    for (auto& side : history_heuristic_)
    {
        for (auto& piece : side)
        {
            for (auto& sqr : piece)
            {
                sqr /= 2;
            }
        }
    }

    for (auto& prev_piece : cont_history_)
    {
        for (auto& prev_to : prev_piece)
        {
            for (auto& piece : prev_to)
            {
                for (auto& sqr : piece)
                {
                    sqr /= 2;
                }
            }
        }
    }
}

// Capture Moves are given a score with MVV_LVA in Generation, this handles other cases.
void Search::BoostMoves(Board& board, MoveList& current_moves, uint8_t ply)
{
    Color ally_color = board.GetCurStatus().cur_color_;
    const ContRow* cont1 = ContHistRow(ply, 1);
    const ContRow* cont2 = ContHistRow(ply, 2);

    for (size_t i{}; i < current_moves.move_count; ++i)
    {
        auto& move = current_moves.list[i];
        
        if (board.GetPieceOnSquare(move.GetTo()) != Piece::kNone && !SeeGe(board, move, 0)) // Bad captures
        {
            move.score_ += kBadCapturePenalty; // If its a capture test the SEE and if it does not pass the threshold add a bad capture penalty.
        }
        else if (move == killers_[ply][0]) // Killer moves
        { 
            move.score_ += kKiller1Boost;
        }
        else if (move == killers_[ply][1])
        {
            move.score_ += kKiller2Boost; 
        }
        else if (board.GetPieceOnSquare(move.GetTo()) == Piece::kNone) // Quiet and not Killer check its history for a score.
        {
            const auto piece = static_cast<size_t>(board.GetPieceOnSquare(move.GetFrom()));
            const auto to_sqr = static_cast<size_t>(move.GetTo());

            // need to move to int32_t since we are adding 3 moves.
            int32_t raw = history_heuristic_[static_cast<size_t>(ally_color)][piece][to_sqr] * kHistoryMoveBoostCeiling; // raw history.
            if (cont1 != nullptr) { raw += ((*cont1)[piece][to_sqr] * kHistoryPly1BoostCeiling); } // 1 ply back boost
            if (cont2 != nullptr) { raw += ((*cont2)[piece][to_sqr] * kHistoryPly2BoostCeiling); } // 2 ply back boost

            // rescale score into ceiling
            move.score_ = raw / kHistoryMax;
        }
    }
}

bool Search::SeeGe(Board& board, ExtMove move, MoveScore threshold) 
{
    // Promotions and en passant change what sits on the square on arrival.
    if (move.GetType() != MoveType::kNormal) { return true; }

    const Square from = move.GetFrom();
    const Square to = move.GetTo();

    int balance = PieceValue(board.GetPieceOnSquare(to)) - threshold;
    if (balance < 0) { return false; }   // a free victim still misses the threshold

    balance = PieceValue(board.GetPieceOnSquare(from)) - balance;
    if (balance <= 0) { return true; }   // holds even if the capturer is lost

    Bitboard occ = board.GetColorBB(Color::kAll) & ~ToBitboard(from) & ~ToBitboard(to);
    Bitboard attackers = attacks::AttacksTo(board, to, occ);

    const Bitboard bishops_queens = board.GetPieceBB(Piece::kBishop) | board.GetPieceBB(Piece::kQueen);
    const Bitboard rooks_queens = board.GetPieceBB(Piece::kRook) | board.GetPieceBB(Piece::kQueen);

    Color stm = board.GetCurStatus().cur_color_;
    bool verdict = true;

    while (true)
    {
        stm = GetEnemyColor(stm);
        attackers &= occ;  // drop revealed sliders that have since been captured

        const Bitboard mine = attackers & board.GetColorBB(stm);
        if (mine == 0) { break; }  // the speculated recapture never happens

        verdict = !verdict; // confirm it did happen

        // Least valuable attacker first.
        Piece piece = Piece::kNone;
        for (const Piece candidate : { Piece::kPawn, Piece::kKnight, Piece::kBishop, Piece::kRook, Piece::kQueen, Piece::kKing })
        {
            if ((mine & board.GetPieceBB(candidate)) != 0) { piece = candidate; break; }
        }

        if (piece == Piece::kKing)
        {
            // A king may only recapture when nothing enemy is still aiming here.
            if ((attackers & board.GetColorBB(GetEnemyColor(stm))) != 0) { verdict = !verdict; }
            break;
        }

        balance = PieceValue(piece) - balance;   // speculate the next recapture
        if (balance < (verdict ? 1 : 0)) { break; }

        occ ^= ToBitboard(GetLsb(mine & board.GetPieceBB(piece)));

        // Reveal sliders that were hiding behind what we just removed. A knight is
        // never on a line through the target, so it can expose nothing.
        if (piece == Piece::kPawn || piece == Piece::kBishop || piece == Piece::kQueen)
        {
            attackers |= attacks::GetBishopAttack(occ, to) & bishops_queens;
        }
        if (piece == Piece::kRook || piece == Piece::kQueen)
        {
            attackers |= attacks::GetRookAttack(occ, to) & rooks_queens;
        }
    }

    return verdict;
}


void PickNextMove(MoveList& current_moves, size_t cur_index)
{
    MoveScore best_score = std::numeric_limits<MoveScore>::min();
    size_t best_move = cur_index;

    for (size_t i{cur_index}; i < current_moves.move_count; ++i)
    {
        if (current_moves.list[i].score_ > best_score)
        {
            best_move = i;
            best_score = current_moves.list[i].score_;
        }
    }
    std::swap(current_moves.list[cur_index], current_moves.list[best_move]);
}

void Search::CountNode()
{
    nodes_searched++;
    if (use_node_limit_ && ++total_nodes_ >= node_limit_) { stop_flag_ = true; }
}

Score Search::QuesSearch(Board& board, Score alpha, Score beta, uint8_t depth, uint8_t ply)
{
    // check for draw 50 moves or 3 fold and return 0.
    CountNode();
    if (board.IsDraw()) { return kDrawScore; }

    between::SetCheckersAndBlockers(board, board.GetCurStatus());
    const bool is_check = board.IsCheck();
    
    Score standing_pat = evaluation::Evaluate(board, board.GetCurStatus().cur_color_);
    Score best_score{};
    auto& status = board.GetCurStatus();

    if (is_check)
    {
        if (depth == 0 || stop_flag_) { return standing_pat; }
        best_score = kNegInf;   // forced to move, cannot stand pat
    }
    else
    {
        if (standing_pat >= beta || depth == 0 || stop_flag_) { return standing_pat; }
        
        // Delta pruning check the captured piece and if it is not worth it () skip. Turn of in end game.
        if (status.phase_num > 14)
        {
            Score big_delta = kPieceValues[static_cast<size_t>(Piece::kQueen)];
            
            if ((board.GetColoredPieceBB(status.cur_color_, Piece::kPawn) & ToBitboard((status.cur_color_ == Color::kWhite) ? Row::_7 : Row::_2)) != 0)
            {
                big_delta += 700;
            }

            if (standing_pat < alpha - big_delta)
            {
                return standing_pat;
            }
        }

        alpha = std::max(standing_pat, alpha);
        best_score = standing_pat;
    }

    MoveList ques_moves;
    if (is_check)
    {
        generation::legal::GenerateMoves<GenType::kAll>(board, ques_moves);
    }
    else 
    {
        generation::legal::GenerateMoves<GenType::kNoisy>(board, ques_moves);
    }
    
    size_t legal_count{};
    for (size_t i{}; i < ques_moves.move_count; ++i)
    {
        PickNextMove(ques_moves, i);
        ExtMove move = ques_moves.list[i];

        bool legal = generation::legal::IsGeneratedMoveLegal(board, status, move, status.blocking_squares_, status.cur_color_, GetEnemyColor(status.cur_color_));
        if (!legal) { continue; }

        legal_count++;

        if (!is_check && !SeeGe(board, move, 0)) { continue; }

        moving::MakeMove(board, move);
        Score score = -QuesSearch(board, -beta, -alpha, depth - 1, ply + 1);
        moving::UndoMove(board, move);

        if (score >= beta)
        {
            return score; 
        }

        alpha = std::max(score, alpha);
        best_score = std::max(score, best_score);
    }

    if (is_check && legal_count == 0) { return static_cast<Score>(kMateScore + ply); } // mate

    return best_score;
}

ContRow* Search::ContHistRow(uint8_t ply, uint8_t back)
{
    if (ply < back) { return nullptr; }
    const PieceTo& prev = move_stack_[ply - back];
    if (prev.piece == Piece::kNone) { return nullptr; }   // root, or a null move
    return &cont_history_[static_cast<size_t>(prev.piece)][static_cast<size_t>(prev.to)];
}


void Search::UpdateHistory(Board& board, ExtMove move, MoveScore bonus, uint8_t ply)
{
    const auto piece  = static_cast<size_t>(board.GetPieceOnSquare(move.GetFrom()));
    const auto to_sqr = static_cast<size_t>(move.GetTo());

    ApplyGravity(history_heuristic_[static_cast<size_t>(board.GetCurStatus().cur_color_)][piece][to_sqr], bonus);

    ContRow* cont1 = ContHistRow(ply, 1);
    ContRow* cont2 = ContHistRow(ply, 2);

    if (cont1 != nullptr) { ApplyGravity((*cont1)[piece][to_sqr], bonus); }
    if (cont2 != nullptr) { ApplyGravity((*cont2)[piece][to_sqr], bonus); }
}

void Search::UpdateKillers(Board& board, ExtMove move, uint8_t depth, uint8_t ply)
{
    // On a non capture move we update the history heuristic and killer move if needed.
    if (board.GetPieceOnSquare(move.GetTo()) != Piece::kNone) { return; }

    // Killers are moves that caused a beta cutoff at a specific ply.
    if (!(killers_[ply][0] == move))
    {
        killers_[ply][1] = killers_[ply][0];
        killers_[ply][0] = move;
    }
}

// Min and max player are combined into one, where alpha is equal to the current players score and beta is equal to the opposing player score.
// Alpha will be better if it is greater than the opposing player, so for black we need to flip the evaluation * -1.
// So greater score will be better no matter color.
Score Search::NegaMax(Board& board, Score alpha, Score beta, uint8_t depth, uint8_t ply, bool nmp_flag) // NOLINT
{   
    const size_t eval_s_i = ply + kEvalStackOffset;
    pv_length_[ply] = 0;
    prev_evals_[eval_s_i] = kNegInf;
    CountNode();

    // check for 50 move rule and 3 fold repitition
    if (ply > 0 && board.IsDraw()) { return kDrawScore; }
    
    if (depth == 0|| ply >= (kMaxPly - kQuesDepth - 1)) { return QuesSearch(board, alpha, beta, kQuesDepth, ply); } // check extension, currently no guard later add a more mechanical guard for stopping depth blowups.

    Score best_score = kNegInf;
    bool best_score_set = false;
    ExtMove best_move{};
    Score orig_alpha = alpha;
    auto& status = board.GetCurStatus();
    Color enemy_color = GetEnemyColor(status.cur_color_);
    between::SetCheckersAndBlockers(board, status); // set checker blockers for each position.
    bool is_check = board.IsCheck();
    
    if (is_check) { depth++; }

    // TT based cutoff
    TEntry tt_entry = ttable_->GetEntry(status.hash_);
    bool tt_entry_exists = !tt_entry.IsEmpty(); 
    if (tt_entry_exists && tt_entry.GetDepth() >= depth ) // prove for cutoff.
    {  
        Score tt_score = TableToScore(tt_entry.GetScore(), ply);
        // Exact means this value was evaluated completely inside the tree (no cutoffs), so no need to search it again.
        if (tt_entry.GetType() == NodeType::EXACT) { return TableToScore(tt_entry.GetScore(), ply); }

        // Fail low or the upper bound of that branch. Any move in this branch did not exceed alpha so if the saved
        // score of this move is less than current alpha we dont need to search again.
        if (tt_entry.GetType() == NodeType::UPPER && tt_score <= alpha) { return TableToScore(tt_entry.GetScore(), ply);  }

        // Fail high or the lower bound for that branch. Previously this move was at least this score, it could be
        // more. So if it exceeds our current beta same thing will happen so we can return early.
        if (tt_entry.GetType() == NodeType::LOWER && tt_score >= beta) { return TableToScore(tt_entry.GetScore(), ply);  }
    }

    Score static_eval{kNegInf};
    // Reverse futlity prunning, if currently not in check and static eval is above beta + margin, not pv node, and not in mate threshold (then it doesnt matter). We can prune.
    {   
        if (!is_check)
        {
            static_eval = evaluation::Evaluate(board, status.cur_color_);
            if (depth <= kRFP_MaxDepth && (beta <= alpha + 1) && std::abs(beta) < kMateScoreThreshold && 
                (static_eval - (kRFP_Margin * depth)) >= beta)
            {
                return static_eval;
            }
        }
    }

    // Here is the last point before possibly going down the tree so update prev_eval.
    prev_evals_[eval_s_i] = static_eval;
    bool is_improving = (prev_evals_[eval_s_i - 2] < prev_evals_[eval_s_i]);

    // Null Move Prunning. Only apply, when depth is relevant, not in check, cur side has a non pawn piece, previous move was not null move and also only on non pv nodes.
    {
        if (depth >= kNMP_MinDepth && !is_check && !nmp_flag && (beta <= alpha + 1)
                && ((board.GetColorBB(status.cur_color_) & ~board.GetColoredPieceBB(status.cur_color_, Piece::kPawn) & ~board.GetColoredPieceBB(status.cur_color_, Piece::kKing)) != 0))
        {
            int npr_reduction = kNMP_Base + (depth / kNMP_DepthDivisor); // base amount to reduce, + how much more to reduce with at higher depths.
            
            uint8_t reduced_depth = std::max((depth - npr_reduction - 1), 0);

            move_stack_[ply] = {Piece::kNone, Square{}};
            moving::MakeNullMove(board); // make the null move
            between::SetCheckersAndBlockers(board, board.GetCurStatus()); // reset checker blockers for now.
            Score nmp_score = -NegaMax(board, -beta, -beta + 1, reduced_depth, ply + 1, true);
            moving::UndoNullMove(board);
            
            if (nmp_score >= beta)
            {
                if (std::abs(nmp_score) > kMateScoreThreshold) { nmp_score = beta; }
                return nmp_score; 
            }
        }
    }


    // TT Move check
    bool is_hashmove_capture = false;
    if (tt_entry_exists)
    {
        // If no prune but still entry, check the stored move first, returns true if causes a beta cutoff. Else updates alpha/best_score and best_move.
        auto tt_move = static_cast<ExtMove>(tt_entry.GetMove());
        if (generation::legal::IsStoredMoveLegal(board, tt_move)) 
        {
            move_stack_[ply] = {board.GetPieceOnSquare(tt_move.GetFrom()), tt_move.GetTo()};
            moving::MakeMove(board, tt_move);
            Score tt_move_score = -NegaMax(board, -beta, -alpha, depth - 1, ply + 1, false);
            moving::UndoMove(board, tt_move);

            is_hashmove_capture = !(board.GetPieceOnSquare(tt_move.GetTo()) == Piece::kNone);
            // current score exceeded the acceptable threshhold of the opposing player, so no need to search rest of branch.
            if (tt_move_score >= beta && !stop_flag_)
            {
                // if tt move is quiet then update its history as well.
                if ((tt_move.GetType() == MoveType::kNormal) && !is_hashmove_capture)
                {
                    UpdateKillers(board, tt_move, depth, ply);
                    UpdateHistory(board, tt_move, std::clamp<MoveScore>(static_cast<MoveScore>(depth * depth), -kHistoryMax, kHistoryMax), ply);
                }
    
                ttable_->AddEntry({board.GetCurStatus().hash_, ScoreToTable(tt_move_score, ply), tt_move, depth, NodeType::LOWER, tt_age_});
                return tt_move_score;
            }

            // if score is now exceeding alpha this is a new high score, adjust alpha and also update the pv
            best_score = tt_move_score;
            best_move  = tt_move;
            best_score_set = true;
            if (tt_move_score > alpha)
            {
                alpha = tt_move_score;
                UpdatePv(ply, tt_move);
            }
        }
    }

    // stop flag check, can do every node since it is atomic check.
    if (stop_flag_) { return 0; }

    MoveList possible_moves;
    generation::legal::GenerateMoves<GenType::kAll>(board, possible_moves); 
    BoostMoves(board, possible_moves, ply);

    // After getting the score we can add this position to Transposition table.
    size_t legal_move_count{};
    MoveScore history_bonus = std::clamp<MoveScore>(static_cast<MoveScore>(depth * depth), -kHistoryMax, kHistoryMax);
    
    std::array<ExtMove, 64> searched_quiets{};
    size_t searched_quiet_count{};

    // Run through all the moves
    for (size_t i{}; i < possible_moves.move_count; ++i)
    {
        PickNextMove(possible_moves, i);
        ExtMove move = possible_moves.list[i];
        bool legal = generation::legal::IsGeneratedMoveLegal(board, status, move, status.blocking_squares_, status.cur_color_, enemy_color);
        if (!legal) { continue; }
        
        legal_move_count++;
        
        // Skip the tt move we already searched.
        if (tt_entry_exists && (move == tt_entry.GetMove())) { continue; }

        Piece move_piece = board.GetPieceOnSquare(move.GetFrom());
        bool is_quiet = (move.GetType() == MoveType::kNormal || (move.GetType() == MoveType::kCastle)) 
                        && (board.GetPieceOnSquare(move.GetTo()) == Piece::kNone);
        
        // Futility Prunning 
        // At edge nodes if this node has no chance to increase alpha then prune it.
        // Needs to not be in check, be a quiet move, be below selected depth, fail 
        // to increase alpha with margin that scales with depth, and not inside mate range
        { 
            if (best_score_set && is_quiet && !is_check && (beta <= alpha + 1) && depth <= kFutilityMaxDepth 
            && (std::abs(alpha) < kMateScoreThreshold) && ((static_eval + depth * kFutilityMargin) <= alpha))
            {
                continue;
            }
        }


        // Late Move Prunning
        // Moves beyond a certain threhsold in list are skipped entirely.
        // Prereq is that it cant be pv, is quiet, no check, and only searched
        // At lower depths. 
        {
            if (best_score_set && is_quiet && !is_check && (beta <= alpha + 1) && depth <= kLMP_MaxDepth && best_score > -kMateScoreThreshold)
            {
                // actual decision maker since legal_move count controls how many moves we are into list.
                if (legal_move_count >= static_cast<size_t>(kLMP_Base + (depth * depth))) 
                {
                    continue;
                }
            }
        }
        
        move_stack_[ply] = {board.GetPieceOnSquare(move.GetFrom()), move.GetTo()};
        moving::MakeMove(board, move);

        ttable_->PrefetchEntry(board.GetCurStatus().hash_); // prefech entry

        // PVS or Scout, If we have a set score scout ahead and test if the move is pv, if it is then redo the search with wider bounds
        // If it isnt then we escape with very small window and dont have to pay for entire search.
        Score score{};
        if (!best_score_set)  // If no previous bounds then just do a full search.
        {
            score = -NegaMax(board, -beta, -alpha, depth - 1, ply + 1, false); // full window
            best_score_set = true;
        }
        else // if we have some bounds then we can do a reduction if it passes our criteria with pvs.
        {
            // Late Move Reduction, for moves furthur down in order reduce depth.
            // Skip reduction on low depths, skip if in top 3 moves.
            int8_t reduction{0};

            if (depth >= 3 && legal_move_count >= 3 && is_quiet && (beta <= alpha + 1))
            {
                auto hist_score = history_heuristic_[static_cast<size_t>(status.cur_color_)][static_cast<size_t>(move_piece)][static_cast<size_t>(move.GetTo())];
                auto hist_red = -((hist_score * kLMR_HistReduction) / (kHistoryMax)); //  if history is bad increase reduction if its good reduce reduction. max is 2.

                double reduction_ratio = kLMR_Base + ((log(depth) * log(legal_move_count)) / kLMR_Divisor);

                reduction = std::round(reduction_ratio + hist_red);

                //reduction += is_check ? -1 : 0; // if prev posisiton is in check reduce reduction
            
                //reduction += (beta <= alpha + 1) ? 0 : -1; // if move is pv reduce less

                //reduction += is_improving ? -1 : 0; // if move is improving reduce it less.

                //reduction += is_hashmove_capture ? 1 : 0; // increase reduction if hash move is capture.

                reduction = std::clamp<int8_t>(reduction, 0, depth - 2);
                // clamp redcution to b e either 1 or until we skip depthbut not into quesseince search instantly
            }

            score = -NegaMax(board, (-alpha - 1), -alpha, (depth - 1 - reduction), ply + 1, false);

            if (reduction > 0 && score > alpha) // lmr fails do a scout on the search.
            {
                score = -NegaMax(board, (-alpha - 1), -alpha, depth - 1, ply + 1, false);
            }
            if (score > alpha && score < beta) // scout failed, go ahead and run full search since its pv
            {
                score = -NegaMax(board, -beta, -alpha, depth - 1, ply + 1, false); // full window
            }
        }
        
        moving::UndoMove(board, move);

        // current score exceeded the acceptable threshhold of the opposing player, so no need to search rest of branch.
        if (score >= beta && !stop_flag_)
        {
            // if move is quiet, update the killers for this ply and also update history of this move with a bonus. 
            // For quiet moves prior to this one that didnt cause a cutoff should be penalized os in future will go after this move.
            if (is_quiet) 
            { 
                UpdateKillers(board, move, depth, ply); 
                UpdateHistory(board, move, history_bonus, ply);

                for (size_t q{}; q < searched_quiet_count; ++q)
                {
                    UpdateHistory(board, searched_quiets[q], -history_bonus, ply);
                }
            }
            ttable_->AddEntry({board.GetCurStatus().hash_, ScoreToTable(score, ply), move, depth, NodeType::LOWER, tt_age_});
            return score; 
        }


        // if score is now exceeding alpha this is a new high score, adjust alpha, update pv, and best score.
        const bool raised_alpha = (score > alpha);

        if (score > best_score)
        {
            best_score = score;
            best_move  = move;
        }

        if (raised_alpha)
        {
            alpha = score;
            UpdatePv(ply, move);
        }

        if (is_quiet && searched_quiet_count < searched_quiets.size())
        {
            searched_quiets[searched_quiet_count++] = move;
        }
    }

    // If 0 moves possible in all if we are in check it is mate and if not then it is a stalemate
    if (legal_move_count == 0) { return is_check ? static_cast<Score>(kMateScore + ply) : kDrawScore; }
    NodeType type = NodeType::UPPER;
    
    if (best_score > orig_alpha) { type = NodeType::EXACT; } // Exceeded alpha but lower than bet, its exactly inside our range.
    
    if (!stop_flag_) { ttable_->AddEntry({board.GetCurStatus().hash_, ScoreToTable(best_score, ply), best_move, depth, type, tt_age_}); }

    return best_score;
};


std::pair<ExtMove, Score> Search::NegaMaxWrapper(Board& board, int depth, Score alpha, Score beta, ExtMove prev_best)
{
    prev_evals_[0 + kEvalStackOffset] = evaluation::Evaluate(board, board.GetCurStatus().cur_color_);
    Score best_score{kNegInf};
    ExtMove best_move{};
    bool best_score_set{false};

    MoveList possible_moves;
    between::SetCheckersAndBlockers(board, board.GetCurStatus());
    generation::legal::GenerateMoves<GenType::kAll>(board, possible_moves); 
    generation::legal::CleanMoveList(board, possible_moves);

    if (possible_moves.move_count == 0 || stop_flag_) 
    { 
        if (board.IsCheck())
        {
            return {best_move, kMateScore}; // mate
        }
        return {best_move, kDrawScore};  //stalemate
    } 

    pv_length_[0] = 0;
    for (size_t i{}; i < possible_moves.move_count; ++i)
    {
        if (possible_moves.list[i] == prev_best)
        {
            possible_moves.list[i].score_ = kRefutationMoveBoost;
            break;
        }
    }

    int searched_moves_count{0};
    for (size_t i{}; i < possible_moves.move_count; ++i)
    {
        PickNextMove(possible_moves, i);
        ExtMove move = possible_moves.list[i];
        
        move_stack_[0] = {board.GetPieceOnSquare(move.GetFrom()), move.GetTo()};
        moving::MakeMove(board, move);
        ttable_->PrefetchEntry(board.GetCurStatus().hash_); // prefech entry

        // pvs
        Score score{};
        if (!best_score_set)  // If no previous bounds then just do a full search.
        {
            score = -NegaMax(board, -beta, -alpha, depth - 1, 1, false); // full window
            best_score_set = true;
        }
        else // if we have some bounds do pvs,
        {
            score = -NegaMax(board, (-alpha - 1), -alpha, depth - 1, 1, false);

            if (score > alpha && score < beta) // scout failed, go ahead and run full search since its pv
            {
                score = -NegaMax(board, -beta, -alpha, depth - 1, 1, false); // full window
            }
        }

        // Since we are going to call the generation of the opposing player we need to switch them and swap signs like in NegaMax.
        moving::UndoMove(board, move);
        
        if (stop_flag_) { break; }

        if (score >= beta) { best_score = score; best_move = move; break; } // breaks aspiration window return now to widen the window.

        if (score > best_score)
        {
            best_score = score;
            best_move = move;
            alpha = std::max(alpha, score);
            UpdatePv(0, move);
        }
    }

    return {best_move, best_score};
};


// Adding iterative deepening. Bascially instead of straight up searching to specified depth we search at depth 1 until that depth.
// This is a a bit reduntant but after adding a Transposition table we acheive more cutoffs due to skipping paths which are incredibly bad.
ExtMove Search::FindBestMove(Board& board, int64_t soft_ms, size_t depth, size_t start_depth, size_t id_, uint64_t node_limit)
{
    node_limit_ = node_limit;
    use_node_limit_ = node_limit > 0;
    total_nodes_ = 0;

    Score alpha = kNegInf;
    Score beta = kPosInf;
    ExtMove best_move{};

    if (++tt_age_ == 0) // age wrapped around
    {
        ttable_->ResetAges(); // stale everything.
        tt_age_ = 1;
    }

    auto search_start = std::chrono::high_resolution_clock::now();
    int64_t prev_iter_time = 0;

    AgeHistory();
    prev_evals_.fill(kNegInf);
    for (size_t i{start_depth}; i <= depth; i++)
    {
        auto iter_start = std::chrono::high_resolution_clock::now();
        Score delta = 25; // 1/4 of the pawn worth
        nodes_searched = 0;
        std::pair<ExtMove, Score> move_score_pair;

        while (!stop_flag_) // aspiration window, start at previous found score. If the score is outside the bounds research else move up.
        {
            move_score_pair = NegaMaxWrapper(board, i, alpha, beta, best_move);

            if (move_score_pair.second <= alpha)
            {
                beta = static_cast<Score>((alpha + beta) / 2);
                alpha = move_score_pair.second - delta;
            }
            else if (move_score_pair.second >= beta)
            {
                beta = move_score_pair.second + delta;
            }
            else
            {
                break;
            }

            delta += delta / 2;
        }

        if (i > 3)
        {
            alpha = move_score_pair.second - delta;
            beta = move_score_pair.second + delta;
        }

        // Test if we can run another iteration
        auto now = std::chrono::high_resolution_clock::now();
        int64_t elapsed = (now - search_start).count() / 1000000;
        int64_t iter_time = (now - iter_start).count() / 1000000;

        if (!stop_flag_)
        {
            // print uci info
            best_move = move_score_pair.first;
            if (id_ == 0)
            {
                std::cout 
                << "info depth " << i 
                << " score cp " << move_score_pair.second
                << " nodes " << nodes_searched
                << " nps " << ((elapsed > 0) ? (static_cast<int64_t>(nodes_searched) * 1000 / elapsed) : 0)
                << " time " << elapsed
                << " pv";
    
                for (int i{}; i < pv_length_[0]; ++i) // print pv
                {
                    std::cout << " " << MoveToString(pv_table_[0][i]);
                }
        
                std::cout << "\n";
            }
        }

        if (stop_flag_) { break; }

        double ebf = (prev_iter_time > 0) ? std::clamp(static_cast<double>(iter_time) / prev_iter_time, 1.5, 4.0) : 2.0;

        if (elapsed + (iter_time * ebf) > soft_ms) { break; }
        prev_iter_time = iter_time;
    }

    return best_move;
};


void Search::ResetSearch()
{
    history_heuristic_ = {};
    cont_history_ = {};
    killers_ = {};
    pv_table_ = {};
    pv_length_ = {};
    killers_ = {};
    ttable_->ClearTT();
    prev_evals_ = {};

    tt_age_ = 0;
}
