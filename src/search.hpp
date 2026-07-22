#pragma once

#include "board.hpp"
#include "move.hpp"
#include "tt.hpp"
#include <condition_variable>
#include <memory>


class Search
{
    Board& board_;
    std::condition_variable stop_flag_;
    
    Score QuesSearch(Score alpha, Score beta, uint8_t depth);
    
    void SortMoveList(MoveList& move_list);
    Move NegaMaxWrapper(int depth);
    Score NegaMax(Score alpha, Score beta, uint8_t depth);
    
public:
    std::unique_ptr<TTable> ttable_;
    uint64_t nodes_searched{};
    
    Move FindBestMove(int depth);

    explicit Search(Board& board);
};