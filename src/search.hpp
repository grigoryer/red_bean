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

public:
    std::unique_ptr<TTable> ttable_;

private:
    
    Score QuesSearch(Score alpha, Score beta, uint8_t depth);
    Score NegaMax(Score alpha, Score beta, uint8_t depth, uint8_t ply);
    std::pair<ExtMove, Score> NegaMaxWrapper(int depth, Score alpha, Score beta);
    
public:
    uint64_t nodes_searched{};
    
    ExtMove FindBestMove(size_t depth = kMaxSearchDepth);
    explicit Search(Board& board, std::atomic<bool>& stop_flag);
};