#include "search.hpp"
#include "board.hpp"
#include "debug.hpp"
#include "global.hpp"
#include "move.hpp"
#include "move_generation.hpp"
#include "move_making.hpp"
#include "eval.hpp"
#include <algorithm>
#include <limits>


constexpr Score kNegInf = std::numeric_limits<Score>::min() + 1;
constexpr Score kPosInf = std::numeric_limits<Score>::max();

Search::Search(Board& board) : board_(board) {}




// First implementation
Score Search::MaxPlayer(Score alpha, Score beta, size_t depth) // NOLINT
{
    if (depth == 0) { return Evaluation::EvaluatePos(board_); }
    Score max_score = kNegInf;

    MoveList possible_moves;
    
    generation::legal::GenerateMoves<GenType::kNoisy>(board_, possible_moves);
    generation::legal::GenerateMoves<GenType::kQuiet>(board_, possible_moves);

    if (possible_moves.move_count == 0)
    {
        return max_score;
    }

    for (size_t i{}; i < possible_moves.move_count; ++i)
    {
        Move move = possible_moves.list[i];
        nodes_searched++;
        moving::MakeMove(board_, move);
        Score score = MinPlayer(alpha, beta, depth - 1);
        moving::UndoMove(board_, move);

        // beta is best score the min player can garnetee himself, so if we exceed that score we dont need to check this branch anymore
        if (score >= beta) 
        {
            return score;
        }
        if (score > alpha) // NOLINT
        {
            alpha = score;
        }

        if (score > max_score) // NOLINT
        {
            max_score = score;
        }
    }

    return max_score;
};

Score Search::MinPlayer(Score alpha, Score beta, size_t depth) // NOLINT
{
    if (depth == 0) { return Evaluation::EvaluatePos(board_); }
    Score min_score = kPosInf;

    MoveList possible_moves;
    
    generation::legal::GenerateMoves<GenType::kNoisy>(board_, possible_moves);
    generation::legal::GenerateMoves<GenType::kQuiet>(board_, possible_moves);


    if (possible_moves.move_count == 0)
    {
        return min_score;
    }

    for (size_t i{}; i < possible_moves.move_count; ++i)
    {
        nodes_searched++;
        Move move = possible_moves.list[i];

        moving::MakeMove(board_, move);
        Score score = MaxPlayer(alpha, beta, depth - 1);
        moving::UndoMove(board_, move);

        //prune branches since the maximizing player can already garentee a better score somewhere else.
        // so basically we never go to this position since it is atleast worse than the other option
        if (score <= alpha) 
        {
            return score;
        }
        
        //prune branches since the maximizing player can already garentee a better score somewhere else.
        if (score < beta) // NOLINT
        {
            beta = score;
        }

        if (score < min_score) // NOLINT
        {
            min_score = score;
        }
    }
    return min_score;
} 


// Min and max player are combined into one, where alpha is equal to the current players score and beta is equal to the opposing player score.
// Alpha will be better if it is greater than the opposing player, so for black we need to flip the evaluation * -1.
// So greater score will be better no matter color.
Score Search::NegaMax(Score alpha, Score beta, size_t depth) // NOLINT
{
    if (depth == 0) { return (Evaluation::EvaluatePos(board_) * (board_.GetCurStatus().cur_color_ == Color::kBlack ? -1 : 1)); }

    Score best_score = kNegInf;

    MoveList possible_moves;
    
    generation::legal::GenerateMoves<GenType::kNoisy>(board_, possible_moves);
    generation::legal::GenerateMoves<GenType::kQuiet>(board_, possible_moves);

    // mate or stalemate return min score, for now no need to compare stalemate since we only want to win :).
    // later add change to change min score depending on how far away the mate is.
    if (possible_moves.move_count == 0) { return kNegInf; }

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
            return score;
        }
        
        // if score is now exceeding alpha this is a new high score, adjust alpha.
        if (score > alpha) // NOLINT
        {
            alpha = score;
        }
        // adjust best score.
        if (score > best_score) // NOLINT
        {
            best_score = score;
        }
    }

    return best_score;
};


Move Search::FindBestMove(int depth)
{
    Move best_move{};
    nodes_searched = 0;
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
        std::cout << "\nMove: "; PrintMove(move);
        std::cout << "Score: " << score;
    }

    std::cout << "\n\nBest Move: "; PrintMove(best_move);
    std::cout << "Best Score: " << best_score;

    return best_move;
};





//old implementaion for alpha beta
/*
Move Search::FindBestMove(int depth)
{
    Move best_move{};
    nodes_searched = 0;

    MoveList possible_moves;
    generation::legal::GenerateMoves<GenType::kNoisy>(board_, possible_moves);
    generation::legal::GenerateMoves<GenType::kQuiet>(board_, possible_moves);

    if (board_.GetCurStatus().cur_color_ == Color::kWhite)
    {
        Score best_score = kNegInf;
        for (size_t i{}; i < possible_moves.move_count; ++i)
        {
            nodes_searched++;
            Move move = possible_moves.list[i];
            moving::MakeMove(board_, move);
            Score score = MinPlayer(kNegInf, kPosInf, depth - 1);
            moving::UndoMove(board_, move);

            if (score > best_score)
            {
                best_score = score;
                best_move = move;
            }
            std::cout << "\nMove: "; PrintMove(move);
            std::cout << "Score: " << score;
        }

        std::cout << "\n\nBest Move: "; PrintMove(best_move);
        std::cout << "Best Score: " << best_score;
    }
    else
    {
        Score best_score = kPosInf;
        for (size_t i{}; i < possible_moves.move_count; ++i)
        {
            Move move = possible_moves.list[i];
            moving::MakeMove(board_, move);
            Score score = MaxPlayer(kNegInf, kPosInf, depth - 1);
            moving::UndoMove(board_, move);

            if (score < best_score)
            {
                best_score = score;
                best_move = move;
            }

            std::cout << "\nMove: "; PrintMove(move);
            std::cout << "Score: " << score;
        }


        std::cout << "\n\nBest Move: "; PrintMove(best_move);
        std::cout << "Best Score: " << best_score;
    }
    return best_move;
};
*/