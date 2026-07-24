#include "debug.hpp"
#include "board.hpp"
#include "eval.hpp"
#include "global.hpp"
#include "move.hpp"
#include "perft.hpp"
#include "move_making.hpp"
#include "move_generation.hpp"
#include <atomic>
#include <chrono>
#include <cstddef>
#include "search.hpp"
#include "uci.hpp"
#include <iostream>
#include <ratio>
#include <string>


auto TestSearchSpeed(Board board, Search& searcher)
{

    PrintPieceBoard(board);
    PrintStatus(board.GetCurStatus());
    auto start = std::chrono::high_resolution_clock::now();
    Move move = searcher.FindBestMove(6);
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "\nNodes: " << searcher.nodes_searched;
    std::cout << "\nTime: " << elapsed.count() << "ms";

    std::cout << "\n\nTable Hits: " << searcher.ttable_->node_hits;
    std::cout << "\nTable Mises: " << searcher.nodes_searched - searcher.ttable_->node_hits;
    std::cout << "\nHit Rate: " << 100 * static_cast<double>(searcher.ttable_->node_hits) /  static_cast<double>(searcher.nodes_searched);

    PrintStatus(board.GetCurStatus());
    searcher.ttable_->ProfileTableSpread();

    return elapsed;
}


int main() 
{    
    UCI engine{};
    //Board board{kPerft3};
    //std::atomic<bool> flag{false};
    //Search searcher(board, flag);
    engine.Loop();
   
    // while(std::cin.get())
    // {
    //     PrintPieceBoard(board);
    //     PrintStatus(board.GetCurStatus());
    //     ExtMove move = searcher.FindBestMove(7);
    //     moving::MakeMove(board, move);
    //     PrintPieceBoard(board);
    //     PrintMove(move);
    //     std::cout << Evaluation::EvaluatePos(board);
    // }

    return 0;
}