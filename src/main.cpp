#include "debug.hpp"
#include "board.hpp"
#include "global.hpp"
#include "move.hpp"
#include "perft.hpp"
#include "move_making.hpp"
#include "move_generation.hpp"
#include <cstddef>
#include <iostream>


int main()
{
    
    Board board{};
    
    PrintPieceBoard(board);
    PrintStatus(board.GetCurStatus());

    perft::PerftDivide(board, 6);

    PrintStatus(board.GetCurStatus());

    return 0;
}