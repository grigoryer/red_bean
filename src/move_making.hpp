#pragma once

#include "move.hpp"
#include "board.hpp"
#include <cstddef>


namespace moving
{
    void make_move(Board& board, Move move);
    void undo_move(Board& board, Move move);
}