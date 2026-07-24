#pragma once

#include "move.hpp"
#include "board.hpp"
#include <cstddef>


namespace moving
{
    void MakeMove(Board& board, ExtMove move);
    void UndoMove(Board& board, ExtMove move);
}