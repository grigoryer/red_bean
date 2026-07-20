#pragma once

#include "move.hpp"
#include "board.hpp"
#include <cstddef>


namespace moving
{
    void MakeMove(Board& board, Move move);
    void UndoMove(Board& board, Move move);
}