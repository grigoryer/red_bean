#include "search.hpp"
#include "board.hpp"
#include "checking.hpp"
#include "eval_constants.hpp"
#include "global.hpp"
#include "move.hpp"
#include "move_generation.hpp"
#include "move_making.hpp"
#include "eval.hpp"
#include "search_globals.hpp"
#include "tt.hpp"
#include "uci.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>

Search::Search(Board& board, std::atomic<bool>& stop_flag)
    : board_(board), stop_flag_(stop_flag), ttable_(std::make_unique<TTable>()) { }


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
    for (auto& by_color : history_heuristic_)
    {
        for (auto& by_from : by_color)
        {
            for (auto& value : by_from) { value /= 2; }
        }
    }
}

void Search::BoostMoves(MoveList& current_moves, uint8_t ply)
{
    Color ally_color = board_.GetCurStatus().cur_color_;
    for (size_t i{}; i < current_moves.move_count; ++i)
    {
        auto& move = current_moves.list[i];
        
        if (move == killers_[ply][0])
        { 
            move.score_ += kKiller1Boost;
        }
        else if (move == killers_[ply][1])
        {
            move.score_ += kKiller2Boost; 
        }
        else if (board_.GetPieceOnSquare(move.GetTo()) == Piece::kNone)
        {
            const MoveScore raw = history_heuristic_[static_cast<size_t>(ally_color)]
                                            [static_cast<size_t>(move.GetFrom())]
                                            [static_cast<size_t>(move.GetTo())];

            move.score_ += static_cast<MoveScore>((raw * kHistoryCeiling) / kHistoryMax);
        }
    }
}

void PickNextMove(MoveList& current_moves, size_t cur_index)
{
    MoveScore best_score = 0;
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


// the three TT bound checks. True => caller returns `out`.
bool Search::ProbeTTCutoff(const TEntry& entry, Score alpha, Score beta, uint8_t depth, Score& out, uint8_t ply) const
{
    Score tt_score = entry.score_;
    if (entry.depth_ >= depth)
    {
        // Exact means this value was evaluated completely inside the tree (no cutoffs), so no need to search it again.
        if (entry.type_ == NodeType::EXACT) { out = tt_score; return true; }

        // Fail low or the upper bound of that branch. Any move in this branch did not exceed alpha so if the saved
        // score of this move is less than current alpha we dont need to search again.
        if (entry.type_ == NodeType::UPPER && tt_score <= alpha) { out = tt_score; return true; }

        // Fail high or the lower bound for that branch. Previously this move was at least this score, it could be
        // more. So if it exceeds our current beta same thing will happen so we can return early.
        if (entry.type_ == NodeType::LOWER && tt_score >= beta) { out = tt_score; return true; }
    }
    return false;
}

bool Search::SearchTTMove(ExtMove tt_move, Score& alpha, Score beta, uint8_t depth, uint8_t ply, Score& best_score, ExtMove& best_move, const Status& status, Score& out)
{
    moving::MakeMove(board_, tt_move);
    Score tt_move_score = -NegaMax(-beta, -alpha, depth - 1, ply + 1, false);

    moving::UndoMove(board_, tt_move);

    // current score exceeded the acceptable threshhold of the opposing player, so no need to search rest of branch.
    if (tt_move_score >= beta && !stop_flag_)
    {
        ttable_->AddEntry({board_.GetCurStatus().hash_, tt_move_score, tt_move, depth, NodeType::LOWER, age});
        out = tt_move_score;
        return true;
    }
    // if score is now exceeding alpha this is a new high score, adjust alpha and also update the pv
    best_score = tt_move_score;
    best_move  = tt_move;
    if (tt_move_score > alpha)
    {
        alpha = tt_move_score;
        UpdatePv(ply, tt_move);
    }
    return false;
}


Score Search::QuesSearch(Score alpha, Score beta, uint8_t depth, uint8_t ply)
{
    // check for draw 50 moves or 3 fold and return 0.
    if (board_.IsDraw()) { return kDrawScore; }

    between::SetCheckersAndBlockers(board_, board_.GetCurStatus());
    const bool is_check = board_.IsCheck();

    
    Score standing_pat = (evaluation::Evaluate(board_) * (board_.GetCurStatus().cur_color_ == Color::kBlack ? -1 : 1));
    Score best_score{};
    auto& status = board_.GetCurStatus();

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
            
            if ((board_.GetColoredPieceBB(status.cur_color_, Piece::kPawn) & ToBitboard((status.cur_color_ == Color::kWhite) ? Row::_7 : Row::_2)) != 0)
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
        generation::legal::GenerateMoves<GenType::kAll>(board_, ques_moves);
    }
    else 
    {
        generation::legal::GenerateMoves<GenType::kNoisy>(board_, ques_moves);
    }
    
    size_t legal_count{};
    for (size_t i{}; i < ques_moves.move_count; ++i)
    {
        PickNextMove(ques_moves, i);
        ExtMove move = ques_moves.list[i];

        bool legal = generation::legal::IsGeneratedMoveLegal(board_, status, move, status.blocking_squares_, status.cur_color_, GetEnemyColor(status.cur_color_));
        if (!legal) { continue; }

        legal_count++;
        nodes_searched++;

        moving::MakeMove(board_, move);
        Score score = -QuesSearch(-beta, -alpha, depth - 1, ply + 1);
        moving::UndoMove(board_, move);

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


void Search::UpdateQuietHeuristics(ExtMove move, uint8_t depth, uint8_t ply)
{
    // On a non capture move we update the history heuristic and killer move if needed.
    if (board_.GetPieceOnSquare(move.GetTo()) != Piece::kNone) { return; }

    if (!(killers_[ply][0] == move)) // update killers
    {
        killers_[ply][1] = killers_[ply][0];
        killers_[ply][0] = move;
    }

    auto& entry = history_heuristic_[static_cast<size_t>(board_.GetCurStatus().cur_color_)][static_cast<size_t>(move.GetFrom())][static_cast<size_t>(move.GetTo())];
    entry = static_cast<MoveScore>(entry + (depth * depth));
    
    if (entry >= kHistoryMax) { AgeHistory(); }
}



// Min and max player are combined into one, where alpha is equal to the current players score and beta is equal to the opposing player score.
// Alpha will be better if it is greater than the opposing player, so for black we need to flip the evaluation * -1.
// So greater score will be better no matter color.
Score Search::NegaMax(Score alpha, Score beta, uint8_t depth, uint8_t ply, bool nmp_flag) // NOLINT
{   
    pv_length_[ply] = 0;

    // check for 50 move rule and 3 fold repitition
    if (ply > 0 && board_.IsDraw()) { return kDrawScore; }
    
    if (depth == 0) { return QuesSearch(alpha, beta, kQuesDepth, ply); }

    Score best_score = kNegInf; 
    bool best_score_set = false;
    ExtMove best_move{};
    Score orig_alpha = alpha;
    auto& status = board_.GetCurStatus();
    Color enemy_color = GetEnemyColor(status.cur_color_);
    between::SetCheckersAndBlockers(board_, status); // set checker blockers for each position.
    bool is_check = board_.IsCheck();

    // TT based cutoffs 
    bool tt_entry_exists = ttable_->ProbeEntry(status.hash_); // bool if it exist if it doesnt dont do anything.
    TEntry probe_entry{}; // create here so can use it in move ordering.
    
    
    if (tt_entry_exists) // prove for cutoff.
    {
        probe_entry = ttable_->GetEntry(status.hash_);
        Score cutoff{};
        // Check if we can pprune based on tt entry aka PV of this position. If cutoff then return prune this branch.
        if (ProbeTTCutoff(probe_entry, alpha, beta, depth, cutoff, ply)) { return cutoff; }
    }

    // Null Move Prunning. Only apply, when depth is relevant, not in check, cur side has a non pawn piece, previous move was not null move and also only on pv nodes.
    {
        if (depth >= 5 && !is_check && !nmp_flag && (beta <= alpha + 1)
                && ((board_.GetColorBB(status.cur_color_) & ~board_.GetColoredPieceBB(status.cur_color_, Piece::kPawn) & ~board_.GetColoredPieceBB(status.cur_color_, Piece::kKing)) != 0))
        {        
            constexpr int npr_reduction = 3; // can be dynamically adjusted later
            moving::MakeNullMove(board_); // make the null move
            between::SetCheckersAndBlockers(board_, board_.GetCurStatus()); // reset checker blockers for now.
            Score nmp_score = -NegaMax(-beta, -beta + 1, depth - 1 - npr_reduction, ply + 1, true);
            moving::UndoNullMove(board_);

            if (nmp_score >= beta)
            {
                if (std::abs(nmp_score) > kMateScoreThreshold) { nmp_score = beta; }
                return nmp_score; 
            }
        }
    }

    // TT Move check
    if (tt_entry_exists)
    {
        Score cutoff{};
        // If no prune but still entry, check the stored move first, returns true if causes a beta cutoff. Else updates alpha/best_score and best_move.
        auto tt_move = static_cast<ExtMove>(probe_entry.refutation_move_);
        if (generation::legal::IsStoredMoveLegal(board_, tt_move)) // check for legality before moving on.
        {
            if (SearchTTMove(tt_move, alpha, beta, depth, ply, best_score, best_move, status, cutoff)) { return cutoff; }
            best_score_set = true;
        }
    }

    // stop flag check, can do every node since it is atomic check.
    if (stop_flag_) { return 0; }


    MoveList possible_moves;
    generation::legal::GenerateMoves<GenType::kAll>(board_, possible_moves); 
    BoostMoves(possible_moves, ply);

    // After getting the score we can add this position to Transposition table.
    size_t legal_move_count{};
    for (size_t i{}; i < possible_moves.move_count; ++i)
    {
        PickNextMove(possible_moves, i);
        ExtMove move = possible_moves.list[i];
        bool legal = generation::legal::IsGeneratedMoveLegal(board_, status, move, status.blocking_squares_, status.cur_color_, enemy_color);
        if (!legal) { continue; }
        
        legal_move_count++;

        if (tt_entry_exists && (move == probe_entry.refutation_move_)) { continue; }

        nodes_searched++;

        bool is_quiet = (move.GetType() == MoveType::kNormal) && (board_.GetPieceOnSquare(move.GetTo()) == Piece::kNone);
        
        moving::MakeMove(board_, move);    
        // Since we always maximize for each player, we need to swap beta and alpha. Since the beta for maximizing player is the alpha for minimizing player and vice versa.
        // We need to swap signs, since a worse score for current player is a better score for the opposing player.
        // Also we need to flip the score when it is returned since again that is opposing players perspective.
        
        // pvs
        Score score{};
        if (!best_score_set)  // If no previous bounds then just do a full search.
        {
            score = -NegaMax(-beta, -alpha, depth - 1, ply + 1, false); // full window
            best_score_set = true;
        }
        else // if we have some bounds then we can do a reduction if it passes our criteria with pvs.
        {
            int8_t reduction{0};

            if (depth >= 3 && legal_move_count >= 4 && is_quiet && !is_check)
            {
                double reduction_ratio = kLMR_Base + ((log(depth) * log(legal_move_count)) / kLMR_Divisor);
                reduction = std::round(reduction_ratio);
                //reduction = static_cast<int8_t>(1 + static_cast<int>(legal_move_count >= 7)); // switch to correct ofurumala of 0.75 + log(depth)  * log(moves searched) / 2.25 later
                reduction = std::min<int8_t>(reduction, static_cast<int8_t>(depth - 2));
            }

            score = -NegaMax((-alpha - 1), -alpha, (depth - 1 - reduction), ply + 1, false);

            if (reduction > 0 && score > alpha) // lmr fails do a scout on the search.
            {
                score = -NegaMax((-alpha - 1), -alpha, depth - 1, ply + 1, false);
            }
            if (score > alpha && score < beta) // scout failed, go ahead and run full search since its pv
            {
                score = -NegaMax(-beta, -alpha, depth - 1, ply + 1, false); // full window
            }
        }
        
        moving::UndoMove(board_, move);

        // current score exceeded the acceptable threshhold of the opposing player, so no need to search rest of branch.
        if (score >= beta && !stop_flag_)
        {
            UpdateQuietHeuristics(move, depth, ply); // Update the quiet moves history and killer moves.
            ttable_->AddEntry({board_.GetCurStatus().hash_, score, move, depth, NodeType::LOWER, age});
            return score; 
        }
        // if score is now exceeding alpha this is a new high score, adjust alpha, update pv, and best score.
        if (score > best_score)
        {
            best_score = score;
            best_move = move;
            if (score > alpha)
            {
                alpha = score;
                UpdatePv(ply, move);
            }
        }
    }

    // If 0 moves possible in all if we are in check it is mate and if not then it is a stalemate
    if (legal_move_count == 0) { return is_check ? static_cast<Score>(kMateScore + ply) : kDrawScore; }
    NodeType type = NodeType::UPPER;
    
    if (best_score > orig_alpha) { type = NodeType::EXACT; } // Exceeded alpha but lower than bet, its exactly inside our range.
    
    if (!stop_flag_) { ttable_->AddEntry({board_.GetCurStatus().hash_, best_score, best_move, depth, type, age}); }

    return best_score;
};


std::pair<ExtMove, Score> Search::NegaMaxWrapper(int depth, Score alpha, Score beta, ExtMove prev_best)
{
    Score best_score{kNegInf};
    ExtMove best_move{};
    bool best_score_set{false};

    MoveList possible_moves;
    between::SetCheckersAndBlockers(board_, board_.GetCurStatus());
    generation::legal::GenerateMoves<GenType::kAll>(board_, possible_moves); 
    generation::legal::CleanMoveList(board_, possible_moves);

    if (possible_moves.move_count == 0 || stop_flag_) 
    { 
        if (board_.IsCheck())
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
        nodes_searched++;
        PickNextMove(possible_moves, i);
        ExtMove move = possible_moves.list[i];
        moving::MakeMove(board_, move);
    
        // pvs
        Score score{};
        if (!best_score_set)  // If no previous bounds then just do a full search.
        {
            score = -NegaMax(-beta, -alpha, depth - 1, 1, false); // full window
            best_score_set = true;
        }
        else // if we have some bounds do pvs,
        {
            score = -NegaMax((-alpha - 1), -alpha, depth - 1, 1, false);

            if (score > alpha && score < beta) // scout failed, go ahead and run full search since its pv
            {
                score = -NegaMax(-beta, -alpha, depth - 1, 1, false); // full window
            }
        }

        // Since we are going to call the generation of the opposing player we need to switch them and swap signs like in NegaMax.
        moving::UndoMove(board_, move);
        
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
ExtMove Search::FindBestMove(int64_t soft_ms, size_t depth)
{
    Score alpha = kNegInf;
    Score beta = kPosInf;

    ExtMove best_move{};
    ttable_->node_hits = 0;
    ttable_->node_requests = 0;
    age++;

    auto search_start = std::chrono::high_resolution_clock::now();
    int64_t prev_iter_time = 0;

    for (int i{1}; i <= depth; i++)
    {
        auto iter_start = std::chrono::high_resolution_clock::now();

        Score delta = 25; // 1/4 of the pawn worth
        nodes_searched = 0;
        std::pair<ExtMove, Score> move_score_pair;

        while (!stop_flag_) // aspiration window, start at previous found score. If the score is outside the bounds research else move up.
        {
            move_score_pair = NegaMaxWrapper(i, alpha, beta, best_move);

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
    pv_table_ = {};
    pv_length_ = {};
    killers_ = {};
    ttable_->ClearTT();

    age = 0;
}