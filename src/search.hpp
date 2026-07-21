#pragma once

#include "board.hpp"
#include "move.hpp"
#include <condition_variable>


using Score = int32_t;


class Search
{
    Board& board_;
    std::condition_variable stop_flag_;

public:
    uint64_t nodes_searched{};
    Move FindBestMove(int depth);
    Score QuesSearch(Score alpha, Score beta, size_t depth);

    Score MaxPlayer(Score alpha, Score beta, size_t depth);
    Score MinPlayer(Score alpha, Score beta, size_t depth);


    Score NegaMax(Score alpha, Score beta, size_t depth);

    explicit Search(Board& board);
};