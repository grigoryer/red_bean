#include "search.hpp"
#include "board.hpp"
#include "debug.hpp"
#include "global.hpp"
#include "move.hpp"
#include "move_generation.hpp"
#include "move_making.hpp"
#include "eval.hpp"
#include "search_globals.hpp"
#include "tt.hpp"
#include <algorithm>
#include <limits>

constexpr Score kNegInf = std::numeric_limits<Score>::min() + 1;
constexpr Score kPosInf = std::numeric_limits<Score>::max();

Search::Search(Board& board) : board_(board),  ttable_(std::make_unique<TTable>()) { }

void Search::SortMoveList(MoveList& move_list)
{
    // for (auto& move : move_list.list)
    // {
    //     move += kMoveOrderingTable[static_cast<size_t>(board_.GetPieceOnSquare(move.GetFrom()))][static_cast<size_t>(board_.GetPieceOnSquare(move.GetTo()))];
    // }
}

// Min and max player are combined into one, where alpha is equal to the current players score and beta is equal to the opposing player score.
// Alpha will be better if it is greater than the opposing player, so for black we need to flip the evaluation * -1.
// So greater score will be better no matter color.
Score Search::NegaMax(Score alpha, Score beta, uint8_t depth) // NOLINT
{   
    // Need to cut off based on the entry, if we do have one.
    TEntry* probe_entry = ttable_->GetEntry(board_.GetCurStatus().hash_);
    if (probe_entry != nullptr)
    {
        if (probe_entry->depth_ >= depth)
        {
            // Exact means this avalue was evlauted completly inside the tree (no cutoffs), so no need to search it again.
            if (probe_entry->type_ == NodeType::EXACT) { return probe_entry->score_; }
    
            // Fail low or the upper bound of that branch. Any move in this branch did not exceed alpha so if the saved score of this move is 
            // less than current alpha we dont need to search again.
            if (probe_entry->type_ == NodeType::UPPER && probe_entry->score_ <= alpha) { return probe_entry->score_; }
    
            // Fail high or the lower bound for that branch. Previously this move was at least this score, it could be more. So if it exceeds 
            // our current beta same thing will happen so we can return early.
            if (probe_entry->type_ == NodeType::LOWER && probe_entry->score_ >= beta) { return probe_entry->score_; }
        }

    }

    if (depth == 0) { return (Evaluation::EvaluatePos(board_) * (board_.GetCurStatus().cur_color_ == Color::kBlack ? -1 : 1)); }

    Score best_score = kNegInf;
    Move best_move{};
    MoveList possible_moves;
    Score orig_alpha = alpha;
    
    generation::legal::GenerateMoves<GenType::kNoisy>(board_, possible_moves);
    generation::legal::GenerateMoves<GenType::kQuiet>(board_, possible_moves);

    // mate or stalemate return min score, for now no need to compare stalemate since we only want to win :).
    // later add change to change min score depending on how far away the mate is.
    if (possible_moves.move_count == 0) { return kNegInf; }

    // After getting the score we can add this position to Transposition table.
    TEntry* new_entry = ttable_->AddEntry({board_.GetCurStatus().hash_, kNegInf, Move(), depth, NodeType::UPPER});

    for (size_t i{}; i < possible_moves.move_count; ++i)
    {
        nodes_searched++;
        Move move = possible_moves.list[i];

        moving::MakeMove(board_, move);
        
        
        // Since we always maximize for each player, we need to swap beta and alpha. Since the beta for maximizing player is the alpha for minimizing player and vice versa.
        // We need to swap signs, since a worse score for current player is a better score for the opposing player.
        // Also we need to flip the score when it is returned since again that is opposing players perspective.
        Score score = -NegaMax(-beta, -alpha, depth - 1);
    
        moving::UndoMove(board_, move);

        // current score exceeded the acceptable threshhold of the opposing player, so no need to search rest of branch.
        if (score >= beta)
        {
            new_entry->score_ = score;
            new_entry->refutation_move_ = move;
            new_entry->type_ = NodeType::LOWER; // Exceeded beta = lower bound of branch.
            return score; 
        }
        // if score is now exceeding alpha this is a new high score, adjust alpha.
        alpha = std::max(score, alpha);
        if (score > best_score)
        {
            best_score = score;
            best_move = move;
        }
    }
    
    if (best_score > orig_alpha)
    {
        new_entry->type_ = NodeType::EXACT; // Exceeded alpha but lower than bet, its exactly inside our range.
    }
    new_entry->score_ = best_score;
    new_entry->refutation_move_ = best_move;

    return best_score;
};



Move Search::NegaMaxWrapper(int depth)
{
    Move best_move{};
    Score alpha = kNegInf;
    Score beta = kPosInf;

    MoveList possible_moves;
    generation::legal::GenerateMoves<GenType::kNoisy>(board_, possible_moves);
    generation::legal::GenerateMoves<GenType::kQuiet>(board_, possible_moves);

    Score best_score = kNegInf;
    for (size_t i{}; i < possible_moves.move_count; ++i)
    {
        nodes_searched++;
        Move move = possible_moves.list[i];
        moving::MakeMove(board_, move);
        // Since we are going to call the generation of the opposing player we need to switch them and swap signs like in NegaMax.
        Score score = -NegaMax(-beta, -alpha, depth - 1);
        moving::UndoMove(board_, move);

        if (score > best_score)
        {
            best_score = score;
            best_move = move;
        }
    }

    std::cout << "\nDepth: " << depth << "\nBest Move: "; PrintMove(best_move);
    std::cout << "Best Score: " << best_score;

    return best_move;
};


// Adding iterative deepening. Bascially instead of straight up searching to specified depth we search at depth 1 until that depth.
// This is a a bit reduntant but after adding a Transposition table we acheive more cutoffs due to skipping paths which are incredibly bad.
Move Search::FindBestMove(int depth)
{
    Move best_move{};
    nodes_searched = 0;
    ttable_->node_hits = 0;

    for (int i{1}; i <= depth; i++)
    {
        best_move = NegaMaxWrapper(i);
    }

    std::cout << "\n\n\nBest Move: "; 
    PrintMove(best_move);
    return best_move;
};
