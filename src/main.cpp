#include "attacks.hpp"
#include "checking.hpp"
#include "debug.hpp"
#include "board.hpp"
#include "global.hpp"
#include "checking.hpp"
#include "move.hpp"
#include "move_generation.hpp"

#include <iostream>


int main()
{
    Board board{};
    board.ParseFenString("8/2rr1rrr/2rn1n1r/4Kn1r/2rnnn2/2rrr1rr/3rr3/2k5 w - - 0 1");
    between::SetSliderCheckersAndBlockers(board, board.GetCurStatus());
    PrintStatus(board.GetCurStatus());
    
    MoveList list{};

    generation::legal::GenerateMoves<GenType::kNoisy>(board, list);

    PrintMoveList(list);
    PrintPieceBoard(board);

    return 0;
}



/*

*/