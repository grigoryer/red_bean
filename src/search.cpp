#include "search.hpp"
#include "board.hpp"
#include "global.hpp"
#include "move.hpp"
#include "move_generation.hpp"
#include "move_making.hpp"
#include "eval.hpp"
#include "search_globals.hpp"
#include "tt.hpp"
#include "uci.hpp"
#include <algorithm>

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


Score Search::QuesSearch(Score alpha, Score beta, uint8_t depth)
{
    // check for draw 50 moves or 3 fold and return 0.
    if (board_.IsDraw()) { return kDrawScore; }

    Score standing_pat = (evaluation::Evaluate(board_) * (board_.GetCurStatus().cur_color_ == Color::kBlack ? -1 : 1));

    if (standing_pat >= beta || depth == 0 || stop_flag_)
    {
        return standing_pat;
    }

    alpha = std::max(standing_pat, alpha);
    Score best_score = standing_pat;

    MoveList noisy_moves;
    between::SetCheckersAndBlockers(board_, board_.GetCurStatus());
    generation::legal::GenerateMoves<GenType::kNoisy>(board_, noisy_moves);
    auto& status = board_.GetCurStatus();

    for (size_t i{}; i < noisy_moves.move_count; ++i)
    {
        PickNextMove(noisy_moves, i);
        ExtMove move = noisy_moves.list[i];
       
        bool legal = generation::legal::IsGeneratedMoveLegal(board_, status, move, status.blocking_squares_, status.cur_color_, GetEnemyColor(status.cur_color_));
        if (!legal) { continue; }
        
        nodes_searched++;

        
        moving::MakeMove(board_, move);
        Score score = -QuesSearch(-beta, -alpha, depth - 1);
        moving::UndoMove(board_, move);

        if (score >= beta)
        {
            return score; 
        }
        alpha = std::max(score, alpha);
        best_score = std::max(score, best_score);
    }
    return best_score;
}



// the three TT bound checks. True => caller returns `out`.
bool Search::ProbeTTCutoff(const TEntry& entry, Score alpha, Score beta, uint8_t depth, Score& out) const
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
    if (!generation::legal::IsStoredMoveLegal(board_, tt_move)) { return false; }

    moving::MakeMove(board_, tt_move);
    Score tt_move_score = -NegaMax(-beta, -alpha, depth - 1, ply + 1);
    
    moving::UndoMove(board_, tt_move);

    // current score exceeded the acceptable threshhold of the opposing player, so no need to search rest of branch.
    if (tt_move_score >= beta && !stop_flag_)
    {
        ttable_->AddEntry({board_.GetCurStatus().hash_, tt_move_score, tt_move, depth,
                            NodeType::LOWER, status.half_move_});
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


void Search::UpdateQuietHeuristics(ExtMove move, uint8_t depth, uint8_t ply)
{
    // On a non capture move we update the history heuristic and killer move if needed.
    if (board_.GetPieceOnSquare(move.GetTo()) != Piece::kNone) { return; }

    if (!(killers_[ply][0] == move))
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

Score Search::NegaMax(Score alpha, Score beta, uint8_t depth, uint8_t ply) // NOLINT
{   
    pv_length_[ply] = 0;

    // check for 50 move rule and 3 fold repitition
    if (ply > 0 && board_.IsDraw()) { return kDrawScore; }
    
    if (depth == 0) { return QuesSearch(alpha, beta, kQuesDepth); }

    Score best_score = kNegInf; 
    ExtMove best_move{};
    Score orig_alpha = alpha;
    auto& status = board_.GetCurStatus();
    Color enemy_color = GetEnemyColor(status.cur_color_);
    between::SetCheckersAndBlockers(board_, status); // set checker blockers for each position.

    TEntry* probe_entry = ttable_->GetEntry(status.hash_);
    if (probe_entry != nullptr)
    {
        Score cutoff{};
        // Check if we can pprune based on tt entry aka PV of this position. If cutoff then return prune this branch.
        if (ProbeTTCutoff(*probe_entry, alpha, beta, depth, cutoff)) { return cutoff; }

        // If no prune but still entry, check the stored move first, returns true if causes a beta cutoff. Else updates alpha/best_score and best_move.
        auto tt_move = static_cast<ExtMove>(probe_entry->refutation_move_);
        if (SearchTTMove(tt_move, alpha, beta, depth, ply, best_score, best_move, status, cutoff)) { return cutoff; }
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
        if (!legal || (probe_entry != nullptr && move == probe_entry->refutation_move_)) { continue; }
        
        nodes_searched++;
        legal_move_count++;

        moving::MakeMove(board_, move);    
        // Since we always maximize for each player, we need to swap beta and alpha. Since the beta for maximizing player is the alpha for minimizing player and vice versa.
        // We need to swap signs, since a worse score for current player is a better score for the opposing player.
        // Also we need to flip the score when it is returned since again that is opposing players perspective.
        Score score = -NegaMax(-beta, -alpha, depth - 1, ply + 1);
        moving::UndoMove(board_, move);

        // current score exceeded the acceptable threshhold of the opposing player, so no need to search rest of branch.
        if (score >= beta && !stop_flag_)
        {
            UpdateQuietHeuristics(move, depth, ply); // Update the quiet moves history and killer moves.
            ttable_->AddEntry({board_.GetCurStatus().hash_, score, move, depth, NodeType::LOWER, status.half_move_});
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
    if (legal_move_count == 0) { return (board_.GetCurStatus().checking_squares_ != 0) ? static_cast<Score>(kMateScore + ply) : 0; }
    NodeType type = NodeType::UPPER;
    
    if (best_score > orig_alpha) { type = NodeType::EXACT; } // Exceeded alpha but lower than bet, its exactly inside our range.
    
    if (!stop_flag_) { ttable_->AddEntry({board_.GetCurStatus().hash_, best_score, best_move, depth, type, status.half_move_}); }

    return best_score;
};

std::pair<ExtMove, Score> Search::NegaMaxWrapper(int depth, Score alpha, Score beta, ExtMove prev_best)
{
    Score best_score{kNegInf};
    ExtMove best_move{};

    MoveList possible_moves;
    between::SetCheckersAndBlockers(board_, board_.GetCurStatus());
    generation::legal::GenerateMoves<GenType::kAll>(board_, possible_moves); 
    generation::legal::CleanMoveList(board_, possible_moves);

    if (possible_moves.move_count == 0 || stop_flag_) { return {best_move, best_score}; } // mate or stalemate
    if (board_.IsDraw()) { return {best_move, kDrawScore}; }

    pv_length_[0] = 0;
    for (size_t i{}; i < possible_moves.move_count; ++i)
    {
        if (possible_moves.list[i] == prev_best)
        {
            possible_moves.list[i].score_ = kRefutationMoveBoost;
            break;
        }
    }

    for (size_t i{}; i < possible_moves.move_count; ++i)
    {
        nodes_searched++;
        PickNextMove(possible_moves, i);
        ExtMove move = possible_moves.list[i];
        moving::MakeMove(board_, move);
        // Since we are going to call the generation of the opposing player we need to switch them and swap signs like in NegaMax.
        Score score = -NegaMax(-beta, -alpha, depth - 1, 1);
        moving::UndoMove(board_, move);

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
ExtMove Search::FindBestMove(size_t depth)
{
    Score alpha = kNegInf;
    Score beta = kPosInf;

    ExtMove best_move{};
    ttable_->node_hits = 0;
    ttable_->node_requests = 0;

    for (int i{1}; i <= depth; i++)
    {
        Score delta = 25; // 1/4 of the pawn worth
        nodes_searched = 0;
        std::pair<ExtMove, Score> move_score_pair;

        while (!stop_flag_)
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

        if (!stop_flag_)
        {
            best_move = move_score_pair.first;
            std::cout << "info depth " << i
                << " score cp " << move_score_pair.second
                << " nodes " << nodes_searched
                << " pv";
                
            for (int i{}; i < pv_length_[0]; ++i)
            {
                std::cout << " " << MoveToString(pv_table_[0][i]);
            }
            
            std::cout << "\n";
            
        }
    }

    return best_move;
};
