#pragma once

#include "board.hpp"
#include "eval.hpp"
#include "move.hpp"
#include "search_globals.hpp"
#include "tt.hpp"
#include "eval.hpp"
#include <memory>


class Search
{
    Board& board_;
    std::atomic<bool>& stop_flag_;

    std::array<std::array<ExtMove, kMaxPly>, kMaxPly> pv_table_{}; // stores the move list at ply.
    std::array<int, kMaxPly> pv_length_{}; // shows how many of moves in pv are valid.


    std::array<std::array<ExtMove, 2>, kMaxPly> killers_{}; // killer move table, 2 killer moves for ply.
    
    void UpdatePv(uint8_t ply, ExtMove move);
    Score QuesSearch(Score alpha, Score beta, uint8_t depth);
    Score NegaMax(Score alpha, Score beta, uint8_t depth, uint8_t ply);
    std::pair<ExtMove, Score> NegaMaxWrapper(int depth, Score alpha, Score beta, ExtMove prev_best);
    void BoostMoves(MoveList& current_moves, uint8_t ply);
    
public:
    std::unique_ptr<TTable> ttable_;
    uint64_t nodes_searched{};
    
    ExtMove FindBestMove(size_t depth = kMaxSearchDepth);
    explicit Search(Board& board, std::atomic<bool>& stop_flag);
};