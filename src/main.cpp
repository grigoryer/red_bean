#include "attacks.hpp"
#include "debug.hpp"
#include "board.hpp"
#include "global.hpp"

#include <iostream>


int main()
{
    Board board{};
    board.ParseFenString("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");

    PrintBitboard(board.GetColorBB(Color::kAll));
    PrintBitboard(board.GetColorBB(Color::kWhite));
    PrintBitboard(board.GetColorBB(Color::kBlack));

    PrintPieceBoard(board);
    
}